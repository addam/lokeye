#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include <iostream>
#include <ceres/ceres.h>
#include <glog/logging.h>
using ceres::AutoDiffCostFunction;
using ceres::CostFunction;
using ceres::Problem;
using ceres::Solver;
using ceres::Solve;

using Measurements = std::vector<std::pair<Vector2, Vector2>>;

Vector2 project(Vector2 v, const Matrix33 &h)
{
    Vector3 result{v(0), v(1), 1};
    result = h * result;
    return Vector2{result(0) / result(2), result(1) / result(2)};
}

/** Direct linear transform from pair.first to pair.second
 * Beware: the input data is modified (normalized) in the process
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

Vector3 Face::update_step(const Bitmap3 &img, const Bitmap3 &grad, const Bitmap3 &reference, int direction)
{
    Vector3 result = {0, 0, 0};
    Transformation tsf_inv = tsf.inverse();
    for (Pixel p : grad) {
        Vector2 v = grad.to_world(p), refv = tsf_inv(v);
        if (region.contains(to_pixel(refv))) {
            Vector3 diff = img(v) - reference(refv);
            result += diff.dot(grad(p)) * tsf.d(refv, direction);
        }
    }
    Pixel b = *(grad.begin());
    return result;
}

struct LocRot
{
    template <typename T>
    bool operator() (const T p[3], const double v[2], T result[2]) const;
    static void copy(const Transformation &tsf, double params[3]);
    static void copy(const double params[3], Transformation &tsf);
};

class ImageWrapper : public ceres::SizedCostFunction<3, 2>
{
    Vector3 refc;
    const Bitmap3 &image;
    const Bitmap3 &image_dx, &image_dy;
public:
    ImageWrapper(const Bitmap3 &image, Vector3 value, const Bitmap3 &dx, const Bitmap3 &dy);
    virtual bool Evaluate(double const* const*, double*, double**) const;
};

struct UselessWrapper
{
    UselessWrapper(ImageWrapper *w1, Vector2 position);
    template<typename T>
    bool operator()(const T* params, T* residual) const;
private:
    double position[2];
    ceres::CostFunctionToFunctor<3, 2> image_wrapper;
};

template<typename T>
bool LocRot::operator () (const T p[3], const double v[2], T* r) const
{
    T s = sin(p[2]), c = cos(p[2]);
    r[0] = p[0] + c * v[0] - s * v[1];
    r[1] = p[1] + s * v[0] + c * v[1];
    return true;
}

void LocRot::copy(const Transformation &tsf, double params[3])
{
    Vector2 offset = tsf(Vector2(0, 0));
    params[0] = tsf.params[0] + offset[0];
    params[1] = tsf.params[1] + offset[1];
    params[2] = 3 * tsf.params[2] / tsf.static_params[2];
}

void LocRot::copy(const double params[3], Transformation &tsf)
{
    tsf.params[2] = tsf.static_params[2] * params[2] / 3;
    Vector2 offset = tsf(Vector2(0, 0));
    tsf.params[0] = params[0] - offset[0];
    tsf.params[1] = params[1] - offset[1];
}

ImageWrapper::ImageWrapper(const Bitmap3 &image, Vector3 value, const Bitmap3 &dx, const Bitmap3 &dy):
    image(image), refc(value), image_dx(dx), image_dy(dy)
{
}

bool ImageWrapper::Evaluate(double const* const* x, double* residual, double** jacobian) const
{
    Vector2 v(x[0][0], x[0][1]);
    Vector3 img_color = image(v);
    Vector3 img_dx = image_dx(v);
    Vector3 img_dy = image_dy(v);
    for (int i=0; i<3; i++) {
        residual[i] = img_color(i) - refc(i);
        if (jacobian and jacobian[0]) {
            jacobian[0][2*i + 0] = img_dx(i);
            jacobian[0][2*i + 1] = img_dy(i);
        }
    }
    return true;
}

UselessWrapper::UselessWrapper(ImageWrapper *w1, Vector2 _position): image_wrapper(w1)
{
    position[0] = _position[0];
    position[1] = _position[1];
}

template<typename T>
bool UselessWrapper::operator()(const T* params, T* residual) const
{
    T transformed[2];
    LocRot tsf;
    tsf(params, position, transformed);
    return image_wrapper(transformed, residual);
}

void Face::refit(const Bitmap3 &img, bool only_eyes)
{
    using ImgPair = std::pair<Bitmap3, Bitmap3>;
    if (not only_eyes) {
        double params[3];
        LocRot::copy(tsf, params);
        std::vector<ImgPair> pyramid = {std::make_pair(ref(region), img)};
        while (pyramid.back().first.rows > 4 and pyramid.back().first.cols > 4) {
            const ImgPair &pair = pyramid.back();
            pyramid.emplace_back(std::make_pair(pair.first.downscale(), pair.second.downscale()));
        }
        while (not pyramid.empty()) {
            Problem problem;
            const ImgPair &pair = pyramid.back();
            Bitmap3 dx = pair.second.d(0), dy = pair.second.d(1);
            for (Pixel p : pair.first) {
                Vector2 v = pair.first.to_world(p);
                // the library requires me to make three allocations per pixel! How the hell is that supposed to be fast?
                ImageWrapper *w1 = new ImageWrapper(pair.second, pair.first(p), dx, dy);
                UselessWrapper *w2 = new UselessWrapper{w1, v};
                CostFunction* cost_function = new AutoDiffCostFunction<UselessWrapper, 3, 2>(w2);
                problem.AddResidualBlock(cost_function, NULL, params);
            }
            Solver::Options options;
            options.linear_solver_type = ceres::SPARSE_NORMAL_CHOLESKY;
            options.use_nonmonotonic_steps = true;
            options.minimizer_progress_to_stdout = false;
            Solver::Summary summary;
            Solve(options, &problem, &summary);
            pyramid.pop_back();
        }
        printf("result: %g, %g, %g\n", params[0], params[1], params[2]);
        LocRot::copy(params, tsf);
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
