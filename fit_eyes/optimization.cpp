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
    printf(" need %g iterations (%i over %i)\n", result, count_good, count_total);
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

void Face::refit(const Bitmap3 &img, bool only_eyes)
{
    if (not only_eyes) {
        Iterrect rotregion = tsf.inverse()(region);
        Bitmap<Matrix32> grad = gradient(img, rotregion);
        for (int iteration=0; iteration < 30; iteration++) {
            cv::Matx<float, 1, 3> delta_tsf = {0, 0, 0};
            for (Pixel p : rotregion) {
                if (region.contains(tsf(p))) {
                    cv::Matx<float, 3, 1> diff = img(p) - ref(tsf(p));
                    delta_tsf += diff.t() * grad(p) * tsf.grad(p);
                }
            }
            float length = region.max([this, delta_tsf](Pixel corner) { return cv::norm(tsf.grad(corner) * delta_tsf.t()); }, 1e-5);
            //printf("step (%g, %g, %g) / %g\n", grad(0), grad(1), grad(2), length);
            tsf.params -= (1/length) * delta_tsf.t();
        }
    }
    for (Eye &eye : eyes) {
        eye.refit(img, tsf);
    }
}

Vector2 Face::operator () () const
{
    return eyes[0].pos + eyes[1].pos;
}

#ifdef INCOMPLETE
void Eye::init(Bitmap1 &img)
{
    const float max_distance = 2 * radius;
    const int size = 2 * max_distance;
    Bitmap1 votes(size, size, 0.f);
    Pixel offset = to_pixel(init_pos) - Pixel(max_distance, max_distance);
    cv::Rect region(offset, size(votes));
    Bitmap1 dx = img.d(D::X)(region), 
    for (int i=0; i<size; i++) {
        for (int j=0; j<size; j++) {
            
        }
    }
}
#endif

void Eye::refit(const Bitmap3 &img, const Transformation &tsf)
{
    const int iteration_count = 20;
    const float max_distance = 2 * radius;
    if (cv::norm(pos - init_pos) > max_distance) {
        pos = init_pos;
    }
    for (int iteration=0; iteration < iteration_count; iteration++) {
        float weight = (2 * iteration < iteration_count) ? 1 : 1.f / (1 << (iteration / 2 - iteration_count / 4));
        Pixel tsf_pos = to_pixel(tsf(pos));
        Rect bounds(tsf_pos - Pixel(radius, radius), tsf_pos + Pixel(radius, radius));
        Bitmap22 gradgrad = gradient(gradient(grayscale(img, bounds)));
        Vector2 delta_pos = {sum_boundary_dp(gradgrad, 0, tsf), sum_boundary_dp(gradgrad, 1, tsf)};
        //float delta_radius = sum_boundary_dr(img, tsf);
        float step = 1. / std::max({std::abs(delta_pos[0]), std::abs(delta_pos[1])/*, std::abs(delta_radius)*/, 1e-5f});
        //printf("step %g * (move %g, %g; radius %g)\n", step, delta_pos[0], delta_pos[1], delta_radius);
        pos += step * delta_pos;
        //radius += step * delta_radius;
        //radius = std::max(1.f, radius);
    }
}

float Eye::sum_boundary_dp(const Bitmap22 &img, int direction, const Transformation &tsf)
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
            const Matrix22 &value = img(p);
            result += w * (value(0, direction) * dist_x + value(1, direction) * dist_y) / radius;
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
#endif

Iterrect Eye::region(const Transformation &tsf) const
{
    Vector2 center = tsf(pos);
    /// @todo use derivative of tsf wrt. x, y
    float scale = 1;
    return Rect(center[0] - radius * scale, center[1] - radius * scale, 2 * radius * scale + 2, 2 * radius * scale + 2);
}
