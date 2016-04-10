#include "main.h"
using D = Image::Order;

void Face::refit(Image &img)
{
    const char winname[] = "sopel";
    for (int iteration=0; iteration < 10; iteration++) {
        cv::Matx<float, 3, 1> grad = {0, 0, 0};
        Matrix32 tsf_grad = tsf.grad(Vector2(0, 0)).t();
        auto rotregion = Rect(to_pixel(tsf(Vector2(region.x, region.y))), region.size());
        Bitmap3 &sob = ref.d(D::X);
        printf("sobel X size: %i, %i; rotregion %i, %i; %i, %i\n", sob.cols, sob.rows, rotregion.x, rotregion.y, rotregion.width, rotregion.height);
        cv::imshow(winname, sob(region));
        cv::waitKey();
        for (Pixel p : region) {
            Color diff = ref(tsf(p)) - img(p);
            Matrix32 ref_grad = ref.grad(tsf(p));
            cv::Matx<float, 2, 1> tmp = (mask.grad(tsf(p)).t() + mask(tsf(p)) * 2 * diff.t() * ref_grad).t();
            //printf("(%g, %g, %g) * (%g, %g; %g, %g; %g, %g) = (%g; %g)\n", diff[0], diff[1], diff[2], ref_grad(0, 0), ref_grad(0, 1), ref_grad(0, 2), ref_grad(1, 0), ref_grad(1, 1), ref_grad(1, 2), tmp(0), tmp(1));
            grad += diff.dot(diff) * tsf_grad * tmp;
            //printf(" += (%g, %g, %g)^2 * (%g, %g; %g, %g; %g, %g) * (%g; %g)\n", diff[0], diff[1], diff[2], tsf_grad(0, 0), tsf_grad(0, 1), tsf_grad(1, 0), tsf_grad(1, 1), tsf_grad(2, 0), tsf_grad(2, 1), tmp(0, 0), tmp(1, 0));
        }
        float length = region.max([this, grad](Pixel corner) { return cv::norm(tsf.grad(corner) * grad); }, 1e-5);
        printf("step (%g, %g, %g) / %g\n", grad(0), grad(1), grad(2), length);
        tsf.params -= (1/length) * grad;
        exit(14);
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
            result += w * cv::norm(img.d(D::XX, p) * pow2(dist_x) + 2 * img.d(D::XY, p) * dist_x * dist_y + img.d(D::YY, p) * pow2(dist_y * dist_y)) / pow2(radius);
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
