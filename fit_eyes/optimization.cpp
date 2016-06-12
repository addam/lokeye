#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include <iostream>
using Measurements = std::vector<std::pair<Vector2, Vector2>>;

Vector2 project(Vector2 v, const Matrix33 &h)
{
    Vector3 result{v(0), v(1), 1};
    result = h * result;
    return Vector2{result(0) / result(2), result(1) / result(2)};
}

/** Direct linear transform from pair.first to pair.second
 * Beware: the input data is modified (normalized, to be exact) in the process
 */
Matrix33 homography(Measurements &pairs)
{
    Vector2 center_left = {0, 0}, center_right = {0, 0};
    const int count = pairs.size();
    float scale_left = 0, scale_right = 0;
    for (auto pair : pairs) {
        center_left += pair.first / count;
        center_right += pair.second / count;
    }
    for (auto &pair : pairs) {
        pair.first -= center_left;
        pair.second -= center_right;
        scale_left += cv::norm(pair.first) / count;
        scale_right += cv::norm(pair.second) / count;
    }
    for (auto &pair : pairs) {
        pair.first /= scale_left;
        pair.second /= scale_right;
    }
    ColorMatrix system(0, 3);
    for (auto pair : pairs) {
        //std::cout << "pair " << pair.first << " <--> " << pair.second << std::endl;
        Vector3 pst{pair.first(0), pair.first(1), 1};
        ColorMatrix row(1, 3);
        row << Vector3(0, 0, 0), -1 * pst, pair.second(1) * pst;
        system.push_back(row);
        row << 1 * pst, Vector3(0, 0, 0), -pair.second(0) * pst;
        system.push_back(row);
    }
    //std::cout << "solve system: " << system << std::endl;
    Matrix systemf = system.reshape(1);
    Matrix h;
    cv::SVD::solveZ(systemf, h);
    h = h.reshape(0, 3);
    //std::cout << "produces: " << h << std::endl;
    Matrix normalize = (Matrix(3, 3) << 1, 0, -center_left(0), 0, 1, -center_left(1), 0, 0, scale_left);
    Matrix denormalize = (Matrix(3, 3) << scale_right, 0, center_right(0), 0, scale_right, center_right(1), 0, 0, 1);
    return Matrix(denormalize * h * normalize);
}

template<int size>
Measurements random_sample(const Measurements &pairs)
{
    std::array<int, size> indices;
    std::random_device rd;
    std::minstd_rand generator(rd());
    for (int i=0; i<size; i++) {
        int high = pairs.size() - size + i;
        auto it = indices.begin() + i;
        int index = std::uniform_int_distribution<>(0, high - 1)(generator);
        *it = (std::find(indices.begin(), it, index) == it) ? index : high;
    }
    Measurements result;
    result.reserve(size);
    std::transform(indices.begin(), indices.end(), std::back_inserter(result), [pairs](int index) { return pairs[index]; });
    return result;
}

Measurements support(const Matrix33 h, const Measurements &pairs, const float precision)
{
    Measurements result;
    std::copy_if(pairs.begin(), pairs.end(), std::back_inserter(result), [h, precision](std::pair<Vector2, Vector2> p) { return cv::norm(project(p.first, h) - p.second) < precision; });
    return result;
}

template<int count>
float combinations_ratio(int count_total, int count_good)
{
    const float logp = -2;
    float result = logp / std::log(1 - pow2(pow2(count_good / float(count_total))));
    //printf(" need %g iterations (%i over %i)\n", result, count_good, count_total);
    return result;
}

Gaze::Gaze(const Measurements &pairs, int &out_support, float precision)
{
    out_support = 0;
    float iterations = combinations_ratio<4>(pairs.size(), 4);
    for (int i=0; i < iterations; i++) {
        std::vector<std::pair<Vector2, Vector2>> sample = random_sample<4>(pairs);
        size_t prev_sample_size;
        Matrix33 h;
        do {
            prev_sample_size = sample.size();
            h = homography(sample);
            sample = support(h, pairs, precision);
        } while (sample.size() > prev_sample_size);
        if (sample.size() > out_support) {
            out_support = sample.size();
            fn = h;
            iterations = combinations_ratio<4>(pairs.size(), sample.size());
        }
    }
    printf("support = %i / %lu, %f %f %f; %f %f %f; %f %f %f\n", out_support, pairs.size(), fn(0,0), fn(0,1), fn(0,2), fn(1,0), fn(1,1), fn(1,2), fn(2,0), fn(2,1), fn(2,2));
}

Vector2 Gaze::operator () (Vector2 v) const
{
    Vector3 h{v(0), v(1), 1};
    h = fn * h;
    return {h(0) / h(2), h(1) / h(2)};
}

Vector3 Face::update_step(const Bitmap3 &img, const Bitmap3 &grad, const Bitmap3 &reference, int direction) const
{
    Vector3 result = {0, 0, 0};
    // formula : delta_tsf = -sum_pixel (img o tsf - ref)^t * gradient(img o tsf) * gradient(tsf)
    Transformation tsf_inv = tsf.inverse();
    for (Pixel p : grad) {
        Vector2 v = grad.to_world(p), refv = tsf_inv(v);
        if (region.contains(to_pixel(refv))) {
            Vector3 diff = img(v) - reference(refv);
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
float step_length(Vector3 step, Region r, const Transformation &tsf)
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

float line_search(Vector3 delta_tsf, float &max_length, float &prev_energy, const Transformation &tsf, const Bitmap3 &img, const Bitmap3 &ref)
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

void Face::refit(const Bitmap3 &img, bool only_eyes)
{
    if (not only_eyes) {
        const int min_size = 10;
        const int iteration_count = 30;
        const float epsilon = 1e-3;
        Region rotregion = tsf(region);
        std::vector<std::pair<Bitmap3, Bitmap3>> pyramid{std::make_pair(img.crop(rotregion), ref.crop(region))};
        while (pyramid.back().second.rows > min_size and pyramid.back().second.cols > min_size) {
            auto &pair = pyramid.back();
            pyramid.emplace_back(std::make_pair(pair.first.downscale(), pair.second.downscale()));
        }
        while (not pyramid.empty()) {
            auto &pair = pyramid.back();
            Bitmap3 dx = pair.first.d(0), dy = pair.first.d(1);
            float prev_energy = evaluate(tsf, pair.first, pair.second);
            float max_length;
            for (int iteration=0; iteration < iteration_count; ++iteration) {
                Vector3 delta_tsf = update_step(pair.first, dx, pair.second, 0) + update_step(pair.first, dy, pair.second, 1);
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
    for (Eye &eye : eyes) {
        eye.init(img, tsf);
        eye.refit(img, tsf);
    }
}

Vector2 Face::operator () () const
{
    return eyes[0].pos + eyes[1].pos;
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

#ifdef UNUSED
float Eye::sum_boundary_dr(const Bitmap1 &img, const Transformation &tsf)
{
    Vector2 center = tsf(pos);
    /// @todo use derivative of tsf wrt. x, y
    float scale = 1;
    float result = 0;
    float sum_weight = 0;
    for (Pixel p : region(tsf) & img.region()) {
        float dist_x = (p.x - center[0]) / scale;
        float dist_y = (p.y - center[1]) / scale;
        float w = 1 - std::abs(std::sqrt(pow2(dist_x) + pow2(dist_y)) - radius);
        if (w > 0) {
            result += w * cv::sum(img.d(D::XX, p) * pow2(dist_x) + 2 * img.d(D::XY, p) * dist_x * dist_y + img.d(D::YY, p) * pow2(dist_y * dist_y))[0] / pow2(radius);
            sum_weight += w;
        }
    }
    return sum_weight > 0 ? result / sum_weight : 0;
}

Region Eye::region(const Transformation &tsf) const
{
    Vector2 center = tsf(pos);
    /// @todo use derivative of tsf wrt. x, y
    float scale = 1;
    return Rect(center[0] - radius * scale, center[1] - radius * scale, 2 * radius * scale + 2, 2 * radius * scale + 2);
}
#endif
