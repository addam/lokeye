#ifndef MAIN_H
#define MAIN_H
#define WITH_OPENMP
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <array>
#include <vector>
#include <chrono>
#include <string>
#include <thread>
#include <mutex>
#include "vector_math.h"

using std::vector;
using std::array;
using std::string;
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
};

using Triangle = std::array<Vector2, 3>;

/// Container that allows thread-safe access to a data structure
template<typename T>
class Safe : public T {
    std::atomic_flag locked = ATOMIC_FLAG_INIT;
public:
    void lock() {
        while (locked.test_and_set()) {
            std::this_thread::yield();
        }
    }
    void unlock() {
        locked.clear();
    }
};

inline Circle operator* (float coef, const Circle &c)
{
    return {c.center, coef * c.radius};
}

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

inline Vector2 to_vector(cv::Size size)
{
    return Vector2(size.width, size.height);
}

inline Region to_region(Rect rect)
{
    return Region(to_vector(rect.tl()), to_vector(rect.br()));
}

inline Rect to_rect(Region region)
{
    return Rect(to_pixel(region.tl()), to_pixel(region.br()));
}

inline Region to_region(Circle c)
{
    Vector2 margin(c.radius, c.radius);
    return Region(c.center - margin, c.center + margin);
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

class NoFaceException : public std::exception
{
};

#endif // MAIN_H
