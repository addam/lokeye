#ifndef MAIN_H
#define MAIN_H
#define WITH_OPENMP
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <array>
#include <vector>
#include <chrono>
#include "vector_math.h"

using std::vector;
using std::array;
using Pixel = cv::Point;
using cv::VideoCapture;
using cv::Rect;
using Region = cv::Rect_<float>;
using TimePoint = std::chrono::high_resolution_clock::time_point;
using TrackingData = vector<Vector2>;

struct Circle
{
    Vector2 center;
    float radius;
    static Circle average(const vector<Circle>&);
};

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

inline float clamp(float v, float low, float high)
{
    return v < low ? low : high < v ? high : v;
}

template<typename T, typename U = T>
T exchange(T& obj, U&& new_value)
{
    // C++14 example implementation from cppreference.com
    T old_value = std::move(obj);
    obj = std::forward<U>(new_value);
    return old_value;
}

inline array<Vector2, 4> extract_points(Region region)
{
	return {region.tl(), {region.br().x, region.tl().y}, region.br(), {region.tl().x, region.br().y}};
}
#endif // MAIN_H
