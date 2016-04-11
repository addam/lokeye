#include "main.h"
using D = Image::Order;

void Face::refit(Image &img)
{
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
    for (Eye &eye : eyes) {
        eye.refit(img, tsf);
    }
}

void Eye::refit(Image &img, const Transformation &tsf)
{
    for (int iteration=0; iteration < 10; iteration++) {
        Vector2 delta_pos = {sum_boundary_dp(img.d(D::XX), img.d(D::XY), tsf), sum_boundary_dp(img.d(D::XY), img.d(D::YY), tsf)};
        float delta_radius = sum_boundary_dr(img, tsf);
        float step = 1. / std::max({std::abs(delta_pos[0]), std::abs(delta_pos[1]), std::abs(delta_radius), 1e-5f});
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
