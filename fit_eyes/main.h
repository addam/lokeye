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

inline float pow2(float x)
{
    return x*x;
}

struct Iterrect : public Rect {
    Iterrect(const Rect &rect) : Rect{rect} {
    }
    Iterrect operator & (const Iterrect &other) const {
        return Iterrect(static_cast<const Rect&>(*this) & static_cast<const Rect&>(other));
    }
    Iterrect operator & (const Rect &other) const {
        return Iterrect(static_cast<const Rect&>(*this) & other);
    }
    struct Iterator : public Pixel {
        int left, right;
        Iterator(const Pixel &p, int right) : Pixel{p}, left{p.x}, right{right} {
        }
        Pixel& operator *() {
            return *this;
        }
        bool operator != (const Iterator &other) const {
            return y != other.y or x != other.x;
        }
        Iterator& operator++() {
            if (++x >= right) {
                x = left;
                y ++;
            }
            return *this;
        }
    };
    Iterator begin() const {
        return Iterator{tl(), br().x};
    }
    Iterator end() const {
        return Iterator{{tl().x, br().y}, br().x};
    }
    bool contains(Vector2 v) const {
        return Rect::contains(to_pixel(v));
    }
    template<typename Func, typename T>
    T max(Func f, T init) const {
        const Pixel corners[] = {{x, y}, {x+width, y}, {x, y+height}, {x+width, y+height}};
        for (Pixel p : corners) {
            T val = f(p);
            init = std::max(init, val);
        }
        return init;
    }
};

struct Transformation
{
    using Params = Vector3;
    const Vector3 static_params;
    Params params;
    Transformation(Rect region);
    Transformation(Params, Vector3);
    Vector2 operator () (Vector2) const;
    Vector2 operator () (Pixel p) const { return (*this)(to_vector(p)); }
    Rect operator () (Rect) const;
    Transformation inverse() const;
    Matrix23 grad(Vector2) const;
    Matrix23 grad(Pixel p) const { return this->grad(to_vector(p)); }
};

#endif // MAIN_H
