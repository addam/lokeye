#ifndef MAIN_H
#define MAIN_H
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <array>

using cv::Vec2f;
using cv::Vec3f;
using cv::Matx23f;
using cv::Matx32f;
using cv::Mat_;
using Rectf = cv::Rect_<float>;
using cv::VideoCapture;

using Bitmap = Mat_<Vec3f>;

struct Image
{
    enum class Order {
        X = 0,
        Y = 1,
        XX = 2,
        XY = 3,
        YX = 3,
        YY = 4
    };
    Bitmap data;
    std::array<Bitmap, 5> derivatives;
    void read(VideoCapture &cap);
    Vec3f operator () (Vec2f) const;
    Matx32f grad(Vec2f);
    Vec3f d(Order, Vec2f);
};

struct Transformation
{
    using Params = Vec3f;
    Params params;
    Vec2f operator () (Vec2f pos) const;
    Vec2f inverse(Vec2f pos) const;
    Params dx(Vec2f pos) const;
    Params dy(Vec2f pos) const;
    Matx23f grad(Vec2f pos) const;
};

struct Mask
{
    Mat_<float> data;
    float operator () (Vec2f) const;
    Vec2f grad(Vec2f) const;
};

struct Eye
{
    Vec2f pos;
    float radius;
    void refit(const Image&, const Transformation&);
};

struct Face
{
    Rectf region;
    std::array<Eye, 2> eyes;
    Transformation tsf;
    void refit(const Image &img, const Image &ref);
    void render(const Image&);
};

Face mark_eyes(const Image&);

inline cv::Point round(Vec2f vec)
{
    return cv::Point(vec[0], vec[1]);
}

#endif // MAIN_H
