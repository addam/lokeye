#include "main.h"

void Face::refit(const Image &img, const Image &ref)
{
    for (int iteration=0; iteration < 10; iteration++) {
        decltype(tsf.params) grad;
        for (unsigned i=0; i<img.data.rows; i++) {
            for (unsigned j=0; j<img.data.cols; j++) {
                Vec2f pixel = {float(j), float(i)};
                Vec3f diff = ref(tsf(pixel)) - img(pixel);
                grad += diff.dot(diff) * mask.grad(tsf(pixel)) * tsf.grad(pixel) + mask(tsf(pixel)) * 2 * diff.t() * ref.grad(tsf(pixel)) * tsf.grad(pixel);
            }
        }
        length = max(region.corners, [](Vec2f corner) { return (tsf.grad(corner) * grad).norm(); });
        tsf.params -= grad / length;
    }
    for (Eye &eye : eyes) {
        eye.refit(img, tsf);
    }
}

void Eye::refit(const Image &img)
{
    for (int iteration=0; iteration < 100; iteration++) {
        Vec2f delta_pos = {sum_boundary(img.d(XX), img.d(XY)), sum_boundary(img.d(XY), img.d(YY))};
        float delta_radius = dr(img);
        float step = 1. / std::max({std::abs(delta_pos[0]), std::abs(delta_pos[1]), std::abs(delta_radius)});
        pos += step * delta_pos;
        radius += step * delta_radius;
    }
}

Vec3f Eye::sum_boundary(const cv::Mat &img_x, const cv::Mat &img_y)
{
    const int
        top = MAX(c.y - c.r, 0),
        bottom = MIN(c.y + c.r + 2, img_x.rows),
        left = MAX(c.x - c.r, 0),
        right = MIN(c.x + c.r + 2, img_x.cols);
    float result = 0;
    float sum_weight = 0;
    for (int row = top; row < bottom; row++) {
        const float
            *g_row = img_x.ptr<float>(row),
            *h_row = img_y.ptr<float>(row);
        for (int col = left; col < right; col++) {
            float dist_x = col - c.x;
            float dist_y = row - c.y;
            float w = 1 - std::abs(std::sqrt(pow2(dist_x) + pow2(dist_y)) - c.r);
            if (w > 0) {
                result += w * (g_row[col] * dist_x + h_row[col] * dist_y) / c.r;
                sum_weight += w;
            }
        }
    }
    return result / sum_weight;
}

Vec3f Eye::sum_boundary_dr(const Image &img)
{
    float result = 0;
    float sum_weight = 0;
    for (Vec2f pixel : region.crop(img)) {
        float dist_x = pixel.x - pos.x;
        float dist_y = pixel.y - pos.y;
        float w = 1 - std::abs(std::sqrt(pow2(dist_x) + pow2(dist_y)) - radius);
        if (w > 0) {
            result += w * (img.d(XX, pixel) * pow2(dist_x) + 2 * img.d(XY, pixel) * dist_x * dist_y + img.d(YY, pixel) * pow2(dist_y * dist_y)) / pow2(c.r);
            sum_weight += w;
        }
    }
    return result / sum_weight;
}
