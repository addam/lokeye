#include "main.h"
using Image::Order::XX;
using Image::Order::XY;
using Image::Order::YY;

void Face::refit(Image &img)
{
    for (int iteration=0; iteration < 10; iteration++) {
        cv::Matx<float, 3, 1> grad;
        for (Pixel p : region) {
            Color diff = ref(tsf(p)) - img(p);
            grad += (diff.dot(diff) * mask.grad(tsf(p)).t() * tsf.grad(p) + mask(tsf(p)) * 2 * diff.t() * ref.grad(tsf(p)) * tsf.grad(p)).t();
        }
        float length = region.max([this, grad](Pixel corner) { return cv::norm(tsf.grad(corner) * grad); }, 1e-5);
        tsf.params -= (1/length) * grad;
    }
    for (Eye &eye : eyes) {
        eye.refit(img, tsf);
    }
}

void Eye::refit(Image &img, const Transformation &tsf)
{
    for (int iteration=0; iteration < 100; iteration++) {
        Vector2 delta_pos = {sum_boundary_dp(img.d(XX), img.d(XY), tsf), sum_boundary_dp(img.d(XY), img.d(YY), tsf)};
        float delta_radius = sum_boundary_dr(img, tsf);
        float step = 1. / std::max({std::abs(delta_pos[0]), std::abs(delta_pos[1]), std::abs(delta_radius)});
        pos += step * delta_pos;
        radius += step * delta_radius;
    }
}

float Eye::sum_boundary_dp(const Bitmap3 &img_x, const Bitmap3 &img_y, const Transformation &tsf)
{
    Vector2 center = tsf(pos);
    /// @todo use derivative of tsf wrt. x, y
    float scale = 1;
    float result = 0;
    float sum_weight = 0;
    for (Pixel p : region(tsf) & Iterrect(img_x) & Iterrect(img_y)) {
        float dist_x = (p.x - center[0]) / scale;
        float dist_y = (p.y - center[0]) / scale;
        float w = 1 - std::abs(std::sqrt(pow2(dist_x) + pow2(dist_y)) - radius);
        if (w > 0) {
            result += w * cv::norm(img_x(p) * dist_x + img_y(p) * dist_y) / radius;
            sum_weight += w;
        }
    }
    return result / sum_weight;
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
        float dist_y = (p.y - center[0]) / scale;
        float w = 1 - std::abs(std::sqrt(pow2(dist_x) + pow2(dist_y)) - radius);
        if (w > 0) {
            result += w * cv::norm(img.d(XX, p) * pow2(dist_x) + 2 * img.d(XY, p) * dist_x * dist_y + img.d(YY, p) * pow2(dist_y * dist_y)) / pow2(radius);
            sum_weight += w;
        }
    }
    return result / sum_weight;
}

Iterrect Eye::region(const Transformation &tsf) const
{
    Vector2 center = tsf(pos);
    /// @todo use derivative of tsf wrt. x, y
    float scale = 1;
    return Rect(center[0] - radius * scale, center[1] - radius * scale, 2 * radius * scale + 2, 2 * radius * scale + 2);
}
