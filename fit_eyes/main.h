#ifndef MAIN_H
#define MAIN_H
#define WITH_OPENMP
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <array>
#include <vector>
#include <chrono>

using Pixel = cv::Point;
using Vector2 = cv::Vec2f;
using Vector3 = cv::Vec3f;
using Matrix22 = cv::Matx22f;
using Matrix23 = cv::Matx23f;
using Matrix32 = cv::Matx32f;
using Matrix33 = cv::Matx33f;
using cv::VideoCapture;
using cv::Rect;
using Region = cv::Rect_<float>;
using TimePoint = std::chrono::high_resolution_clock::time_point;

using Color = Vector3;
using Matrix = cv::Mat_<float>;
using ColorMatrix = cv::Mat_<Color>;

inline Pixel to_pixel(Vector2 v)
{
    return Pixel{int(v[0]), int(v[1])};
}

inline Vector2 to_vector(Pixel p)
{
    return Vector2{float(p.x), float(p.y)};
}

inline Vector2 to_vector(cv::Point_<float> p)
{
    return Vector2(p.x, p.y);
}

inline Region to_region(Rect rect)
{
    return Region(to_vector(rect.tl()), to_vector(rect.br()));
}

inline Rect to_rect(Region region)
{
    return Rect(to_pixel(region.tl()), to_pixel(region.br()));
}

inline float pow2(float x)
{
    return x*x;
}

struct Transformation
{
    using Params = Vector3;
    const Vector3 static_params;
    Params params;
    Transformation(Region region);
    Transformation(Params, Vector3);
    Transformation operator + (Params) const;
    Transformation& operator += (Params);
    Vector2 operator () (Vector2) const;
    Vector2 operator () (Pixel p) const { return (*this)(to_vector(p)); }
    Region operator () (Region) const;
    Transformation inverse() const;
    Vector2 inverse(Vector2) const;
    Matrix23 grad(Vector2) const;
    Matrix23 grad(Pixel p) const { return this->grad(to_vector(p)); }
    Vector3 d(Vector2, int direction) const;
};

#endif // MAIN_H
