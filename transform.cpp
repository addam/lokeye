//#define WITH_OPENMP
#include <vector>
#include <functional>
#include <tuple>
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/video.hpp"
using cv::Point;
using Vector = cv::Vec3f;
using Bitmap = cv::Mat_<Vector>;
using Matrix = cv::Mat_<float>;

const char *wint = "transform", *winr = "ref", *wind = "depth";

inline float pow2(float x)
{
    return x*x;
}

class Color : public cv::Vec3f
{
public:
    Color(float r, float g, float b) : cv::Vec3f{r, g, b} {
    }
    Color(cv::Vec3f v) : cv::Vec3f{v} {
    }
    const static Color invalid;
    static float difference(Color a, Color b) {
        if (a == invalid or b == invalid) {
            return 1;
        }
        return (std::abs(a[0] - b[0]) + std::abs(a[1] - b[1]) + std::abs(a[2] - b[2])) / 3.0;
    }
};
const Color Color::invalid{-1, -1, -1};

struct Iterrect : public cv::Rect {
    Iterrect(const cv::Rect &rect) : cv::Rect{rect} {
    }
    struct Iterator : public Point {
        int left, right;
        Iterator(const Point &p, int right) : Point{p}, left{p.x}, right{right} {
        }
        Point& operator *() {
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
};

struct Image : public Bitmap
{
    cv::Rect area;
    Image(int rows, int cols) : Bitmap{rows, cols}, area{0, 0, cols, rows} {
    }
    Image(const Bitmap &image) : Bitmap{image}, area{Point{0, 0}, image.size()} {
    }
    static Image read(const std::string &fileName) {
        Bitmap bitmap;
        cv::imread(fileName, cv::IMREAD_COLOR | cv::IMREAD_ANYDEPTH).convertTo(bitmap, CV_32FC3, 1./256);
        return Image(bitmap);
    }
    Point convert(Vector v) const {
        float scale = std::min(area.width, area.height) / 2.f;
        Point center{area.width/2, area.height/2};
        return Point{int(v[0] * scale), int(v[1] * scale)} + center;
    }    
    Color sample(Vector v) const {
        Point p = convert(v);
        return (area.contains(p)) ? (*this)(p) : (Color::invalid);
    }
    Image downscale() const {
        Bitmap subimage;
        cv::pyrDown(*this, subimage);
        return Image{subimage};
    }
};

struct DepthImage : public Image
{
    Matrix depth;
    DepthImage(const Image &image) : Image{image} {
        depth = Matrix::zeros(image.size());
    }
    DepthImage(const Image &image, const Matrix &depth) : Image{image}, depth{depth} {
    }
    Vector convert(Point p) const {
        float z = depth(p);
        float scale = std::min(area.width, area.height) / 2.f;
        Point center{area.width/2, area.height/2};
        p -= center;
        return Vector{p.x / scale, p.y / scale, z};
    }
    DepthImage downscale() const {
        Image subimage = ((Image)(*this)).downscale();
        Matrix subdepth;
        cv::pyrDown(depth, subdepth, subimage.size());
        return DepthImage{subimage, subdepth};
    }
    float compare(Image view, Matrix projection) const {
        const Image &self = *this;
        float sumError = 0, sumWeight = 0;
        std::vector<Vector> w{1};
        Vector &v = w.front();
        for (Point p : Iterrect{self.area}) {
            v = convert(p);
            cv::perspectiveTransform(w, w, projection);
            if (v[2] > 0) {
                Color sample = view.sample(v);
                sumError += Color::difference(sample, self(p));
            }
        }
        return sumError / float(self.size().area());
    }
    float pyrCompare(Image view, Matrix projection, int sizeLimit=20) const {
        const float bias = 1;
        float score = 1 - compare(view, projection);
        printf("compare %g : ", score);
        DepthImage img = downscale();
        do {
            float here = 1 - img.compare(view, projection);
            printf("%g : ", here);
            score = (score + bias) * here;
            img = img.downscale();
            view = view.downscale();
        } while (img.rows >= sizeLimit and img.cols >= sizeLimit);
        return score;
    }
};

using ImagePair = std::pair<DepthImage, Image>;

template<typename State>
class Window
{
    using Callback = std::function<void(Window<State>&)>;
    Callback callback;
    static void onMouse(int event, int x, int y, int flags, void* ptr) {
        Window &self = *((Window*)ptr);
        self.prev = self.mouse;
        self.mouse = {x, y};
        if (event == cv::EVENT_LBUTTONDOWN) {
            self.start = self.mouse;
            self.pressed = true;
        } else if (event == cv::EVENT_LBUTTONUP) {
            self.pressed = false;
        }
        self.ctrl = flags & cv::EVENT_FLAG_CTRLKEY;
        self.alt = flags & cv::EVENT_FLAG_ALTKEY;
        self.shift = flags & cv::EVENT_FLAG_SHIFTKEY;
        self.callback(self);
    }
    const std::string name;
public:
    ImagePair &images;
    State state;
    Point mouse, prev, start;
    cv::Size size;
    bool pressed{false};
    bool ctrl{false}, alt{false}, shift{false};
    Window(std::string name, ImagePair &images, Callback callback) : name{name}, images{images}, callback{callback} {
        cv::namedWindow(name);
        cv::setMouseCallback(name, onMouse, this);
    }
    void show(Bitmap image) {
        size = image.size();
        cv::imshow(name, image);
    }
    void show(Matrix image) {
        size = image.size();
        cv::imshow(name, image);
    }
};

using ProjectionMatrix = std::array<Matrix, 3>;

void viewRotate(Window<ProjectionMatrix> &window)
{
    assert (window.pressed);
    cv::Point2f move = window.mouse - window.prev;
    move *= 2.f / std::max(window.size.width, window.size.height);
    if (window.shift) {
        window.state[2](0, 3) -= move.x;
        window.state[2](1, 3) -= move.y;
    } else if (window.ctrl) {
        window.state[2](2, 3) += move.y;
        Matrix r = Matrix::eye(4, 4);
        r(1, 0) = -(r(0, 1) = move.x);
        r(0, 0) = r(1, 1) = std::sqrt(1 - pow2(move.y));
        window.state[1] = r * window.state[1];
    } else {
        Matrix rx = Matrix::eye(4, 4), ry = Matrix::eye(4, 4);
        rx(0, 2) = -(rx(2, 0) = move.x);
        rx(0, 0) = rx(2, 2) = std::sqrt(1 - pow2(move.x));
        ry(1, 2) = -(ry(2, 1) = move.y);
        ry(1, 1) = ry(2, 2) = std::sqrt(1 - pow2(move.y));
        window.state[1] = rx * ry * window.state[1];
    }
}

Bitmap render(ImagePair images, Matrix projection)
{
    Bitmap result = images.second.clone();
    Matrix zbuffer = 1e8 * Matrix::ones(images.second.size());
    Matrix &depth = images.first.depth;
    std::vector<Vector> w{1};
    Vector &v = w.front();
    for (Point p : Iterrect{images.first.area}) {
        v = images.first.convert(p);
        if (v[2] == 0) {
            continue;
        }
        cv::perspectiveTransform(w, w, projection);
        Point t = images.second.convert(v);
        if (images.second.area.contains(t) and zbuffer(t) > v[2]) {
            result(t) = images.first(p);
            zbuffer(t) = v[2];
        }
    }
    return result;
}

void viewUpdate(Window<ProjectionMatrix> &window)
{
    static float bestScore = 0;
    if (window.pressed) {
        viewRotate(window);
        Matrix projection = window.state[0] * window.state[2] * window.state[1];
        window.show(render(window.images, projection));
        float score = window.images.first.pyrCompare(window.images.second, projection);
        if (score > bestScore) {
            bestScore = score;
            printf("score %g\n", score);
        }
    }
}

void depthPaint(Window<int> &window)
{
    static bool wasPressed{false};
    if (window.pressed or wasPressed) {
        cv::Rect shape{2 * window.start - window.mouse, window.mouse};
        shape &= cv::Rect{Point{0, 0}, window.size};
        window.state = 0;
        if (window.pressed) {
            Bitmap tmp = window.images.first.clone();
            cv::ellipse(tmp, cv::RotatedRect{window.start, shape.size(), 0.f}, 1.f);
            window.show(tmp);
        } else {
            float scale = std::min(shape.width, shape.height) / float(std::min(window.size.height, window.size.width));
            Matrix &depth = window.images.first.depth;
            for (Point p : Iterrect{shape}) {
                Point diff = p - window.start;
                float radius = pow2(diff.x * 2.f / shape.width) + pow2(diff.y * 2.f / shape.height);
                if (radius < 1) {
                    depth(p) += scale * std::sqrt(1 - radius);
                }
            }
            window.show(window.images.first);
        }
        wasPressed = window.pressed;
    }
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: transform (reference.jpg) (view.jpg)\n");
        return 1;
    }
    ImagePair images{Image::read(argv[1]), Image::read(argv[2])};
    Window<int> reference("reference", images, depthPaint);
    reference.show(images.first);
    Window<ProjectionMatrix> view("view", images, viewUpdate);
    view.state[0] = Matrix::eye(4, 4);
    view.state[0](3, 2) = 35.f / ((argc > 3) ? atof(argv[3]) : 30.f);
    view.state[1] = -1 * Matrix::eye(4, 4);
    view.state[2] = Matrix::eye(4, 4);
    view.state[2](2, 3) = -2;
    view.show(images.second);
    do {} while (char(cv::waitKey()) != 27);
    return 0;
}
