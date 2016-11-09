#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include "homography.h"
#include <iostream>

template<int size>
vector<Measurement> random_sample(const vector<Measurement> &pairs)
{
    if (pairs.size() <= size) {
        return vector<Measurement>(pairs.begin(), pairs.end());
    }
    std::array<int, size> indices;
    std::random_device rd;
    std::minstd_rand generator(rd());
    for (int i=0; i<size; i++) {
        int high = pairs.size() - size + i;
        auto it = indices.begin() + i;
        int index = std::uniform_int_distribution<>(0, high - 1)(generator);
        *it = (std::find(indices.begin(), it, index) == it) ? index : high;
    }
    vector<Measurement> result;
    result.reserve(size);
    std::transform(indices.begin(), indices.end(), std::back_inserter(result), [pairs](int index) { return pairs[index]; });
    return result;
}

vector<Measurement> support(const Matrix35 h, const vector<Measurement> &pairs, const float precision)
{
    vector<Measurement> result;
    std::copy_if(pairs.begin(), pairs.end(), std::back_inserter(result), [h, precision](Measurement p) { return cv::norm(project(p.first, h) - p.second) < precision; });
    return result;
}

template<int count>
float combinations_ratio(int count_total, int count_good)
{
    const float logp = -1;
    float result = logp / std::log(1 - pow(count_good / float(count_total), count));
    printf(" need %g samples (%i over %i)\n", result, count_good, count_total);
    return result;
}

Gaze::Gaze(const vector<Measurement> &pairs, int &out_support, float precision)
{
    if (false) {
        vector<Measurement> subpairs = random_sample<8>(pairs);//(pairs.begin(), pairs.begin() + 7);
        fn = homography<3, 5>(subpairs);
        out_support = support(fn, pairs, precision).size();
        printf("support = %i / %lu, %f %f %f %f %f; %f %f %f %f %f; %f %f %f %f %f\n", out_support, pairs.size(), fn(0,0), fn(0,1), fn(0,2), fn(0,3), fn(0,4), fn(1,0), fn(1,1), fn(1,2), fn(0,3), fn(0,4), fn(2,0), fn(2,1), fn(2,2), fn(0,3), fn(0,4));
        return;
    }
    const int necessary_support = std::min(out_support, int(pairs.size()));
    out_support = 0;
    const int min_sample = 7;
    float iterations = 1 + combinations_ratio<min_sample>(pairs.size(), necessary_support);
    for (int i=0; i < iterations; i++) {
        vector<Measurement> sample = random_sample<min_sample>(pairs);
        size_t prev_sample_size;
        Matrix35 h = homography<3, 5>(sample);
        do {
            prev_sample_size = sample.size();
            h = homography<3, 5>(sample);
            sample = support(h, pairs, precision);
        } while (sample.size() > prev_sample_size);
        if (sample.size() > out_support) {
            out_support = sample.size();
            fn = h;
            if (sample.size() >= necessary_support) {
                iterations = combinations_ratio<min_sample>(pairs.size(), sample.size());
            }
        }
    }
    printf("support = %i / %lu, %f %f %f %f %f; %f %f %f %f %f; %f %f %f %f %f\n", out_support, pairs.size(), fn(0,0), fn(0,1), fn(0,2), fn(0,3), fn(0,4), fn(1,0), fn(1,1), fn(1,2), fn(0,3), fn(0,4), fn(2,0), fn(2,1), fn(2,2), fn(0,3), fn(0,4));
}

Vector2 Gaze::operator () (Vector4 v) const
{
    return project(v, fn);
}

Face::Face(const Bitmap3 &ref, Region main_region, Region eye_region, Region nose_region, Eye left_eye, Eye right_eye):
    ref{ref.clone()},
    main_tsf{main_region},
    main_region{main_region},
    eye_region{eye_region},
    nose_region{nose_region},
    eyes{left_eye, right_eye}
{
}

Transformation::Params update_step(const Transformation &tsf, const Bitmap3 &img, const Bitmap3 &grad, const Bitmap3 &ref, int direction)
{
    Transformation::Params result;
    // formula : delta_tsf = -sum_pixel (img o tsf - ref)^t * gradient(img o tsf) * gradient(tsf)
    Transformation tsf_inv = tsf.inverse();
    for (Pixel p : grad) {
        Vector2 v = grad.to_world(p), refv = tsf_inv(v);
        if (ref.contains(refv)) {
            Vector3 diff = img(v) - ref(refv);
            result -= diff.dot(grad(p)) * tsf.d(refv, direction);
        }
    }
    return result;
}

float evaluate(const Transformation &tsf, const Bitmap3 &img, const Bitmap3 &reference)
{
    float result = 0;
    // formula : energy = 1/2 * sum_pixel (img o tsf - ref)^2
    for (Pixel p : reference) {
        Vector2 v = tsf(reference.to_world(p));
        Vector3 diff = img(v) - reference(p);
        result += diff.dot(diff);
    }
    return 0.5 * result;
}

/** Maximum difference caused by adding 'step' to 'tsf', in pixels
 * @param step Differential update of the transformation
 * @param r Region of interest (search domain for the maximum)
 */
float step_length(Transformation::Params step, Region r, const Transformation &tsf)
{
    Vector2 points[] = {to_vector(r.tl()), Vector2(r.tl().x, r.br().y), to_vector(r.br()), Vector2(r.br().x, r.tl().y)};
    float result = 1e-5;
    for (Vector2 v : points) {
        for (int i=0; i<2; i++) {
            result = std::max(result, std::abs(tsf.d(v, i).dot(step)));
        }
    }
    return result;
}

/** Minimum argument of a quadratic function
 * function F satisfies: F(0) = f0, F(1) = f1, F'(0) = df0
 */
float quadratic_minimum(float f0, float f1, float df0)
{
    float second_order = f0 - f1 - df0;
    float minx = -0.5 * df0 / second_order;
    if (second_order < 0) {
        assert (minx < 0);
        assert (f1 < f0);
    }
    if (minx > 1) {
        assert(f1 < f0);
    }
    if (second_order < 0 or minx > 1) {
        return 1;
    } else {
        return 0.9 * minx;
    }
}

float line_search(Transformation::Params delta_tsf, float &max_length, float &prev_energy, const Transformation &tsf, const Bitmap3 &img, const Bitmap3 &ref)
{
    const float epsilon = 1e-5;
    float length = max_length;
    float current_energy = evaluate(tsf + length * delta_tsf, img, ref);
    while (1) {
        float slope = -delta_tsf.dot(delta_tsf) * length;
        float coef = quadratic_minimum(current_energy, current_energy, slope);
        float next_energy = evaluate(tsf + coef * length * delta_tsf, img, ref);
        if (next_energy >= current_energy or coef == 1) {
            max_length = 2 * length;
            prev_energy = current_energy;
            return length;
        } else if (next_energy / current_energy > 1 - epsilon) {
            return 0;
        } else {
            length *= coef;
            current_energy = next_energy;
        }
    }
}

void refit_transformation(Transformation &tsf, Region region, const Bitmap3 &img, const Bitmap3 &ref, int min_size=3)
{
    const int iteration_count = 30;
    Region rotregion = tsf(region);
    vector<std::pair<Bitmap3, Bitmap3>> pyramid{std::make_pair(img.crop(rotregion), ref.crop(region))};
    while (pyramid.back().second.rows >= 2 * min_size and pyramid.back().second.cols >= 2 * min_size) {
        auto &pair = pyramid.back();
        pyramid.emplace_back(std::make_pair(pair.first.downscale(), pair.second.downscale()));
    }
    while (not pyramid.empty()) {
        auto &pair = pyramid.back();
        Bitmap3 dx = pair.first.d(0), dy = pair.first.d(1);
        float prev_energy = evaluate(tsf, pair.first, pair.second);
        float max_length;
        for (int iteration=0; iteration < iteration_count; ++iteration) {
            Transformation::Params delta_tsf = update_step(tsf, pair.first, dx, pair.second, 0) + update_step(tsf, pair.first, dy, pair.second, 1);
            if (iteration == 0) {
                max_length = (1 << pyramid.size()) / step_length(delta_tsf, rotregion, tsf);
            }
            float length = line_search(delta_tsf, max_length, prev_energy, tsf, pair.first, pair.second);
            if (length > 0) {
                tsf += delta_tsf * length;
            } else {
                break;
            }
        }
        pyramid.pop_back();
    }
}

void Face::refit(const Bitmap3 &img, bool only_eyes)
{
    if (not only_eyes) {
        refit_transformation(main_tsf, main_region, img, ref, 5);
        eye_tsf = main_tsf;
        nose_tsf = main_tsf;
        refit_transformation(eye_tsf, eye_region, img, ref, 3);
        refit_transformation(nose_tsf, nose_region, img, ref, 3);
    }
    for (Eye &eye : eyes) {
        eye.init(img, eye_tsf);
        eye.refit(img, eye_tsf);
    }
}

Vector4 Face::operator () () const
{
    const Vector2 e = eyes[0].pos + eyes[1].pos;
    /// @note here, we are assuming that Transformation is linear
    Vector2 difference = main_tsf.inverse(nose_tsf) - main_tsf.inverse(eye_tsf);
    return Vector4(e[0], e[1], difference[0], difference[1]);
}

inline void cast_vote(Bitmap1 &img, Vector2 v, float weight)
{
    Vector2 pos = img.to_local(v);
    const int left = pos(0), right = left + 1, y = pos(1);
    if (left < 0 or right >= img.cols or y < 0 or y >= img.rows) {
        return;
    }
    float* row = img.ptr<float>(y);
    const float lr = pos(0) - left;
    row[left] += weight * (1 - lr);
    row[right] += weight * lr;
}

void Eye::init(const Bitmap3 &img, const Transformation &tsf)
{
    radius = tsf.scale(init_pos) * init_radius;
    const float max_distance = 2 * radius;
    const int size = 2 * max_distance;
    Region region(tsf(init_pos) - Vector2(max_distance, max_distance), tsf(pos) + Vector2(max_distance, max_distance));
    Bitmap1 votes(to_rect(region));
    votes = 0;
    Bitmap1 gray = img.grayscale(region);
    Bitmap1 dx = gray.d(0), dy = gray.d(1);
    for (Pixel p : dx) {
        Vector2 v = dx.to_world(p);
        Vector2 gradient(dx(p), dy(v));
        float size = cv::norm(gradient);
        cast_vote(votes, v - gradient * radius / size, size);
    }
    for (Pixel p : dy) {
        Vector2 v = dy.to_world(p);
        Vector2 gradient(dx(v), dy(p));
        float size = cv::norm(gradient);
        cast_vote(votes, v - gradient * radius / size, size);
    }
    Pixel result;
    cv::minMaxLoc(votes, NULL, NULL, NULL, &result);
    pos = tsf.inverse(votes.to_world(result));
}

void Eye::refit(const Bitmap3 &img, const Transformation &tsf)
{
    const int iteration_count = 5;
    const float max_distance = 2 * radius;
    if (cv::norm(pos - init_pos) > max_distance) {
        pos = init_pos;
    }
    Pixel tsf_pos = to_pixel(tsf(pos));
    Transformation tsf_inv = tsf.inverse();
    const int margin = std::min(int(max_distance), iteration_count) + radius;
    Rect bounds(tsf_pos - Pixel(1, 1) * margin, tsf_pos + Pixel(1, 1) * margin);
    Bitmap1 gray = img.grayscale(bounds);
    Bitmap1 dxx = gray.d(0).d(0), dxy = gray.d(0).d(1), dyy = gray.d(1).d(1);
    for (int iteration=0; iteration < iteration_count; iteration++) {
        float weight = (2 * iteration < iteration_count) ? 1 : 1.f / (1 << (iteration / 2 - iteration_count / 4));
        Vector2 delta_pos = {sum_boundary_dp(dxx, false, tsf) + sum_boundary_dp(dxy, true, tsf), sum_boundary_dp(dxy, false, tsf) + sum_boundary_dp(dyy, true, tsf)};
        //float delta_radius = sum_boundary_dr(img, tsf);
        float length = std::max({std::abs(delta_pos[0]), std::abs(delta_pos[1]), 1e-5f});
        float step = (1.f / (1 << iteration)) / length;
        //printf("step %g * (move %g, %g)\n", step, delta_pos[0], delta_pos[1]);
        pos = tsf_inv(tsf(pos) + step * delta_pos);
        //radius += step * delta_radius;
        //radius = std::max(1.f, radius);
    }
}

float Eye::sum_boundary_dp(const Bitmap1 &derivative, bool is_vertical, const Transformation &tsf)
{
    Vector2 center = tsf(pos);
    /// @todo use derivative of tsf wrt. x, y for distortion into ellipse
    float scale = 1;
    float result = 0;
    float sum_weight = 0;
    for (Pixel p : derivative) {
        Vector2 offcenter = 1./scale * (derivative.to_world(p) - center);
        float w = 1 - std::abs(cv::norm(offcenter) - radius);
        if (w > 0) {
            result += w * (derivative(p) * offcenter(is_vertical ? 1 : 0)) / radius;
            sum_weight += w;
        }
    }
    return sum_weight > 0 ? result / sum_weight : 0;
}
