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
using Matrix2 = cv::Matx22f;
using Matrix3 = cv::Matx33f;
using Matrix23 = cv::Matx23f;
using Matrix32 = cv::Matx32f;
using Matrix = cv::Mat_<float>;
using cv::VideoCapture;
using cv::Rect;
using TimePoint = std::chrono::high_resolution_clock::time_point;

using Color = Vector3;
using Bitmap1 = cv::Mat_<float>;
using Bitmap3 = cv::Mat_<Color>;

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
    Iterrect(const Bitmap3 &bm) : Rect{0, 0, bm.cols, bm.rows} {
    }
    Iterrect operator & (const Iterrect &other) const {
        return Iterrect(static_cast<const Rect&>(*this) & static_cast<const Rect&>(other));
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
    Bitmap3 data;
    std::array<Bitmap3, 5> derivatives;
    Image();
    Image(const Image&);
    bool read(VideoCapture &cap);
    Iterrect region() const;
    Color operator () (Pixel) const;
    Color operator () (Vector2 v) const { return (*this)(to_pixel(v)); }
    Matrix32 grad(Vector2);
    Color d(Order, Vector2);
    Color d(Order o, Pixel p) { return this->d(o, to_vector(p)); }
    Bitmap3& d(Order);
};

struct Transformation
{
    using Params = Vector3;
    const Vector3 static_params;
    Params params;
    Transformation(Rect region);
    Vector2 operator () (Vector2) const;
    Vector2 operator () (Pixel p) const { return (*this)(to_vector(p)); }
    Vector2 inverse(Vector2) const;
    Params dx(Vector2) const;
    Params dy(Vector2) const;
    Matrix23 grad(Vector2) const;
    Matrix23 grad(Pixel p) const { return this->grad(to_vector(p)); }
};

struct Gaze
{
    Matrix3 fn;
    Gaze(const std::vector<std::pair<Vector2, Vector2>>&, int &out_support, float precision=50);
    Vector2 operator () (Vector2) const;
};

struct Mask
{
    Pixel offset;
    Bitmap1 data;
    Mask(cv::Rect region) : offset{region.x, region.y}, data{region.height, region.width} {}
    /// @todo implement!
    float operator () (Vector2) const {return 1;}
    Vector2 grad(Vector2) const {return Vector2{0, 0};}
};

struct Eye
{
    Vector2 pos;
    float radius;
    void refit(Image&, const Transformation&);
protected:
    float sum_boundary_dp(const Bitmap3 &img_x, const Bitmap3 &img_y, const Transformation&);
    float sum_boundary_dr(Image&, const Transformation&);
    Iterrect region(const Transformation&) const;
};

struct Face
{
    Iterrect region;
    std::array<Eye, 2> eyes;
    Image ref;
    Transformation tsf;
    Mask mask;
    Face(const Image &ref, Rect region, Eye left_eye, Eye right_eye) : ref{ref}, tsf{region}, region{region}, eyes{left_eye, right_eye}, mask{region} {
        /// @todo initialize mask
    }
    void refit(Image&, bool only_eyes=false);
    Vector2 operator() () const;
    void render(const Image&) const;
};

Face mark_eyes(Image&);
Gaze calibrate(Face&, VideoCapture&, Pixel window_size=Pixel(1400, 700));

#endif // MAIN_H
