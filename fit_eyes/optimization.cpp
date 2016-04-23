#include "main.h"
#include <iostream>
using D = Image::Order;

Gaze::Gaze(std::vector<std::pair<Vector2, Vector2>> pairs)
{
    Vector2 center_left = {0, 0}, center_right = {0, 0};
    const int count = pairs.size();
    float scale_left = 1, scale_right = 1;
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
    cv::Mat_<Vector3> system(0, 3);
    for (auto pair : pairs) {
        std::cout << "pair " << pair.first << " <--> " << pair.second << std::endl;
        Vector3 pst{pair.first(0), pair.first(1), 1};
        cv::Mat_<Vector3> row(1, 3);
        row << Vector3(0, 0, 0), -1 * pst, pair.second(1) * pst;
        system.push_back(row);
        row << 1 * pst, Vector3(0, 0, 0), -pair.second(0) * pst;
        system.push_back(row);
    }
    std::cout << "solve system: " << system << std::endl;
    cv::Mat_<float> systemf = system.reshape(1);
    cv::Mat_<float> h;
    cv::SVD::solveZ(systemf, h);
    h = h.reshape(0, 3);
    std::cout << "produces: " << h << std::endl;
    //denormalize
    cv::Mat_<float> denor_left = (cv::Mat_<float>(3, 3) << 1, 0, -center_left(0), 0, 1, -center_left(1), 0, 0, scale_left);
    cv::Mat_<float> denor_right = (cv::Mat_<float>(3, 3) << scale_right, 0, center_right(0), 0, scale_right, center_right(1), 0, 0, 1);
    fn = cv::Mat_<float>(denor_right * h * denor_left);
}

Vector2 Gaze::operator () (Vector2 v) const
{
    Vector3 h{v(0), v(1), 1};
    h = fn * h;
    return {h(0) / h(2), h(1) / h(2)};
}

void Face::refit(Image &img, bool only_eyes)
{
    if (not only_eyes) {
        for (int iteration=0; iteration < 30; iteration++) {
            cv::Matx<float, 1, 3> grad = {0, 0, 0};
            auto rotregion = Rect(to_pixel(tsf(Vector2(region.x, region.y))), region.size());
            for (Pixel p : region) {
                cv::Matx<float, 3, 1> diff = img(tsf(p)) - ref(p);
                grad += diff.t() * img.grad(tsf(p)) * tsf.grad(p);
            }
            float length = region.max([this, grad](Pixel corner) { return cv::norm(tsf.grad(corner) * grad.t()); }, 1e-5);
            //printf("step (%g, %g, %g) / %g\n", grad(0), grad(1), grad(2), length);
            tsf.params -= (1/length) * grad.t();
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

void Eye::refit(Image &img, const Transformation &tsf)
{
    const int iteration_count = 20;
    for (int iteration=0; iteration < iteration_count; iteration++) {
        float weight = (2 * iteration < iteration_count) ? 1 : 1.f / (1 << (iteration / 2 - iteration_count / 4));
        Vector2 delta_pos = {sum_boundary_dp(img.d(D::XX), img.d(D::XY), tsf), sum_boundary_dp(img.d(D::XY), img.d(D::YY), tsf)};
        //float delta_radius = sum_boundary_dr(img, tsf);
        float step = 1. / std::max({std::abs(delta_pos[0]), std::abs(delta_pos[1])/*, std::abs(delta_radius)*/, 1e-5f});
        //printf("step %g * (move %g, %g; radius %g)\n", step, delta_pos[0], delta_pos[1], delta_radius);
        pos += step * delta_pos;
        //radius += step * delta_radius;
        //radius = std::max(1.f, radius);
    }
}

float Eye::sum_boundary_dp(const Bitmap3 &img_x, const Bitmap3 &img_y, const Transformation &tsf)
{
    assert (not img_x.empty());
    assert (not img_y.empty());
    Vector2 center = tsf(pos);
    /// @todo use derivative of tsf wrt. x, y
    float scale = 1;
    float result = 0;
    float sum_weight = 0;
    for (Pixel p : region(tsf) & Iterrect(img_x) & Iterrect(img_y)) {
        float dist_x = (p.x - center[0]) / scale;
        float dist_y = (p.y - center[1]) / scale;
        float w = 1 - std::abs(std::sqrt(pow2(dist_x) + pow2(dist_y)) - radius);
        if (w > 0) {
            result += w * cv::sum(img_x(p) * dist_x + img_y(p) * dist_y)[0] / radius;
            sum_weight += w;
        }
    }
    return sum_weight > 0 ? result / sum_weight : 0;
}

float Eye::sum_boundary_dr(Image &img, const Transformation &tsf)
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

Iterrect Eye::region(const Transformation &tsf) const
{
    Vector2 center = tsf(pos);
    /// @todo use derivative of tsf wrt. x, y
    float scale = 1;
    return Rect(center[0] - radius * scale, center[1] - radius * scale, 2 * radius * scale + 2, 2 * radius * scale + 2);
}
