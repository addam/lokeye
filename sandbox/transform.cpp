//#define WITH_OPENMP
#include <vector>
#include <functional>
#include <tuple>
#include <iostream>
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/video.hpp"
using cv::Point;
using Vec2 = cv::Vec2f;
using Vec3 = cv::Vec3f;
using Vec4 = cv::Vec4f;
using Vector = Vec3;
using Bitmap = cv::Mat_<Vector>;
using Matrix = cv::Mat_<float>;
template<int n, int m> using Matf = cv::Matx<float, n, m>;

cv::Matx33f glob_balance;
cv::Vec3f glob_brightness;
bool do_calculate = false;

const int paramCount = 6;

inline float pow2(float x)
{
    return x*x;
}

inline float gonflip(float x)
{
    return std::sqrt(1 - pow2(x));
}

inline Vec2 project2(const Vector &v, const Matrix &m)
{
    float w = m(3, 3) * 1;
    for (int i=0; i<3; i++) {
        w += m(3, i) * v[i];
    }
    if (w == 0) {
        return Vec2{0, 0};
    }
    Vec2 result;
    for (int i=0; i<2; i++) {
        result[i] = m(i, 3);
        for (int j=0; j<3; j++) {
            result[i] += m(i, j) * v[j];
        }
        result[i] /= w;
    }
    return result;
}

inline Vector project3(const Vector &v, const Matrix &m)
{
    float w = m(3, 3) * 1;
    for (int i=0; i<3; i++) {
        w += m(3, i) * v[i];
    }
    if (w == 0) {
        return Vector{0, 0, 0};
    }
    Vector result;
    for (int i=0; i<3; i++) {
        result[i] = m(i, 3);
        for (int j=0; j<3; j++) {
            result[i] += m(i, j) * v[j];
        }
        result[i] /= w;
    }
    return result;
}

Vec4 project4(const Vector &v, const Matrix &m)
{
    Vec4 result;
    for (int i=0; i<4; i++) {
        result[i] = m(i, 3);
        for (int j=0; j<3; j++) {
            result[i] += m(i, j) * v[j];
        }
    }
    return result;    
}

inline Vec4 homogenize(const Vector &v)
{
    return Vec4{v[0], v[1], v[2], 1};
}

inline Vector dehomogenize(const Vec4 &v)
{
    const float c = 1./v[3];
    return Vector{v[0]*c, v[1]*c, v[2]*c};
}

inline bool valid(Vector v)
{
    return v[2] > 0 and v[2] < 1e3;
}

struct ProjectionMatrix
{
    Matrix cam, world, invcam, memory;
    ProjectionMatrix(float fov=30) : cam{Matrix::zeros(4, 4)}, world{Matrix::eye(4, 4)} {
        cam(0, 0) = 1;
        cam(1, 1) = 1;
        cam(2, 3) = 1;
        cam(3, 2) = 35.f / (2 * fov);
        invcam = cam.inv();
    }
    void apply(const Matrix &transform) {
        memory = world.clone();
        world = transform * world;
    }
    void revert() {
        world = memory;
    }
    operator Matrix() const {
        return cam * world * invcam;
    }
};

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

cv::Mat_<Matf<3, 2>> sobel(Bitmap img)
{
    Bitmap partial[2];
    cv::Sobel(img, partial[0], -1, 1, 0, 3);
    cv::Sobel(img, partial[1], -1, 0, 1, 3);
    cv::Mat_<Matf<3, 2>> result{partial[0].size()};
    cv::merge(partial, 2, result);
    const int rows = std::min(partial[0].rows, partial[1].rows), cols = std::min(partial[0].cols, partial[1].cols);
    for (int i=0; i<rows; i++) {
        for (int j=0; j<cols; j++) {
            Matf<3, 2> &d_view_p = result(i, j);
            for (int s=0; s<3; s++) {
                for (int t=0; t<2; t++) {
                    Vec3 colors = partial[t](i, j);
                    d_view_p(s, t) = colors(s);
                }
            }
        }
    }
    return result;
}

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
        float z = 1./depth(p);
        float scale = std::max(area.width, area.height) / 2.f;
        Point center{area.width/2, area.height/2};
        p -= center;
        return Vector{p.x / scale, p.y / scale, z};
    }
    bool hasDepth(Point p) const {
        return depth(p) < 1e5;
    }
    DepthImage downscale() const {
        Image subimage = ((Image)(*this)).downscale();
        Matrix subdepth;
        cv::pyrDown(depth, subdepth, subimage.size());
        return DepthImage{subimage, subdepth};
    }
    int compare(Image view, Matrix projection) const {
        const Image &self = *this;
        float sumError = 0, sumWeight = 0;
        for (Point p : Iterrect{self.area}) {
            if (hasDepth(p)) {
                Vector v = project3(convert(p), projection);
                Color sample = view.sample(v);
                sumError += Color::difference(sample, self(p));
            }
        }
        return sumError / float(self.size().area());
    }
    void collectSamples(Image view, Matrix projection, Matrix &samples, Matrix &results) const {
        const Image &self = *this;
        assert (samples.rows == results.rows);
        for (Point p : Iterrect{area}) {
            if (hasDepth(p)) {
                Vector v = project3(convert(p), projection);
                float weight = 1./size().area();
                Color sample = weight * self(p);
                cv::Vec4f s{sample[0], sample[1], sample[2], weight};
                samples.push_back(s);
                cv::Vec3f color = weight * view.sample(v);
                results.push_back(color);
            }
        }
    }
    float pyrCompare(Image view, Matrix projection, int sizeLimit=20) const {
        Matrix samples{0, 1};
        Matrix results{0, 1};
        collectSamples(view, projection, samples, results);
        DepthImage ref = downscale();
        while (false and ref.rows >= sizeLimit and ref.cols >= sizeLimit) {
            view = view.downscale();
            ref.collectSamples(view, projection, samples, results);
            ref = ref.downscale();
        }
        samples = samples.reshape(1);
        results = results.reshape(1);
        Matrix balance = Matrix::eye(4, 3);
        float totalError = 0;
        for (int i=0; i<3; i++) {
            //cv::solve(samples, results.col(i), balance.col(i), cv::DECOMP_SVD);
            totalError += cv::norm(samples * balance.col(i) - results.col(i), cv::NORM_L2);
        }
        balance = balance.t();
        balance.colRange(0, 3).copyTo(glob_balance);
        balance.col(3).copyTo(glob_brightness);
        //std::cout << balance << std::endl;
        return totalError;
    }
    static Matf<3, 1> psiprime(Matf<3, 1> diff)
    {
        #ifdef PSI_ABSVALUE
        const float epsilon = 1e-4;
        float norm = std::sqrt(pow2(diff(0)) + pow2(diff(3)) + pow2(diff(3)) + epsilon);
        return diff * (1 / norm);
        #else
        return diff;
        #endif
    }
    Matf<paramCount, 1> d_compare(const Image &view, const Matrix &Q, const Matrix d_Q_tr[paramCount]) const
    {
        auto d_E_tr = Matf<paramCount, 1>::zeros(); // result
        const DepthImage &ref = *this;
        Mat_<Matf<3, 2>> d_view = sobel(view);
        int sampleCount = 0;
        for (Point x : Iterrect{area}) {
            if (hasDepth(x)) {
                Vector z3 = ref.convert(x);
                Vec4 z = homogenize(z3);
                Vec4 w = project4(z3, Q);
                Vector w3 = dehomogenize(w);
                Point p = view.convert(w3);
                Matf<3, 2> d_view_p = area.contains(p) ? d_view(p) : Matf<3, 2>::zeros();
                Matf<1, 2> d_E_p = psiprime(view(p) - ref(x)).t() * d_view_p;
                Matf<2, 4> d_p_w = {
                    1 / w[3], 0, 0, -w[0] / pow2(w[3]),
                    0, 1 / w[3], 0, -w[1] / pow2(w[3])};
                for (int k=0; k<paramCount; k++) {
                    Matf<1, 1> scalar = d_E_p * d_p_w * project4(z3, d_Q_tr[k]);
                    d_E_tr(k) += scalar(0);
                }
                sampleCount += 1;
            }
        }
        auto result = d_E_tr * ((sampleCount > 0) ? 1.0f / sampleCount : 0);
        //std::cout << "result = " << result.t() << std::endl;
        return result;
    }
    Matf<paramCount, 1> d_pyrCompare(Image view, ProjectionMatrix projection, int sizeLimit=20) const {
        const Matrix &cam = projection.cam;
        const Matrix invcam = projection.world * projection.invcam;
        const Matrix Q = projection;
        // the derivatives of Q wrt. the six Euclidean transformations
        const Matrix d_Q_tr[] = {
            cam.col(1) * invcam.row(2) - cam.col(2) * invcam.row(1),
            cam.col(0) * invcam.row(2) - cam.col(2) * invcam.row(0),
            cam.col(0) * invcam.row(1) - cam.col(1) * invcam.row(0),
            cam.col(0) * invcam.row(3),
            cam.col(1) * invcam.row(3),
            cam.col(2) * invcam.row(3),
            cam.col(0) * invcam.row(0),
            cam.col(1) * invcam.row(1),
            cam.col(2) * invcam.row(2)
        };
        Matf<paramCount, 1> result = d_compare(view, Q, d_Q_tr);
        DepthImage ref = downscale();
        int layerCount = 0;
        while (ref.rows >= sizeLimit and ref.cols >= sizeLimit) {
            view = view.downscale();
            result += ref.d_compare(view, Q, d_Q_tr);
            layerCount += 1;
            ref = ref.downscale();
        }
        return result * (1.f / layerCount);
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

Matrix parseEuclidean(Matf<6, 1> params)
{
    using cv::SVD;
    using std::sin;
    using std::cos;
    float rot_x = -params(0), rot_y = -params(1), rot_z = -params(2);
    float move_x = params(3), move_y = params(4), move_z = params(5);
    Matrix result{4, 4};
    Matrix init_rotation = (cv::Mat_<float>{3, 3} <<
        1, -rot_z, -rot_y,
        rot_z, 1, -rot_x,
        rot_y, rot_x, 1);
    SVD svd{init_rotation, SVD::MODIFY_A};
    Matrix rotation = svd.u * svd.vt;
    rotation.copyTo(result.colRange(0, 3).rowRange(0, 3));
    Matrix translation = (cv::Mat_<float>{3, 1} << move_x, move_y, move_z);
    translation.copyTo(result.col(3).rowRange(0, 3));
    Matrix homogeneous = (cv::Mat_<float>{1, 4} << 0, 0, 0, 1);
    homogeneous.copyTo(result.row(3));
    return result;
}

Matrix parseEuclidean(Matf<9, 1> params)
{
    using cv::SVD;
    using std::sin;
    using std::cos;
    float sin_x = params(0), sin_y = params(1), sin_z = params(2);
    float move_x = params(3), move_y = params(4), move_z = params(5);
    float diag_x = params(6), diag_y = params(7), diag_z = params(8);
    Matrix result{4, 4};
    Matrix rotation = (cv::Mat_<float>{3, 3} <<
        1 + diag_x, -sin_z, -sin_y,
        sin_z, 1 + diag_y, -sin_x,
        sin_y, sin_x, 1 + diag_z);
    SVD svd{rotation, SVD::MODIFY_A};
    rotation = svd.u * svd.vt;
    rotation.copyTo(result.colRange(0, 3).rowRange(0, 3));
    Matrix translation = (cv::Mat_<float>{3, 1} << move_x, move_y, move_z);
    translation.copyTo(result.col(3).rowRange(0, 3));
    Matrix homogeneous = (cv::Mat_<float>{1, 4} << 0, 0, 0, 1);
    homogeneous.copyTo(result.row(3));
    std::cout << "transformation = " << result << std::endl;
    return result;
}

void viewRotate(Window<ProjectionMatrix> &window)
{
    if (not window.pressed) {
        return;
    }
    cv::Point2f move = window.mouse - window.prev;
    move *= 2.f / std::max(window.size.width, window.size.height);
    auto params = Matf<6, 1>::zeros();
    if (window.shift) {
        params(3) = move.x;
        params(4) = move.y;
    } else if (window.ctrl) {
        params(2) = move.x;
        params(5) = move.y;
    } else {
        params(1) = move.x;
        params(0) = move.y;
    }
    window.state.apply(parseEuclidean(params));
}

Bitmap render(ImagePair images, Matrix projection)
{
    Bitmap result = images.second.clone();
    Matrix zbuffer = Matrix::zeros(images.second.size());
    Matrix &depth = images.first.depth;
    bool have_any{false};
    for (Point p : Iterrect{images.first.area}) {
        if (not images.first.hasDepth(p)) {
            continue;
        }
        Vector v = project3(images.first.convert(p), projection);
        Point t = images.second.convert(v);
        if (images.second.area.contains(t) and zbuffer(t) < v[2]) {
            Color c = images.first(p);
            result(t) = do_calculate ? (glob_balance * c + glob_brightness) : c;
            zbuffer(t) = v[2];
        }
    }
    return result;
}

template<bool force=false>
void viewUpdate(Window<ProjectionMatrix> &window)
{
    static float bestScore = 1e8;
    if (window.pressed or force) {
        viewRotate(window);
        window.show(render(window.images, window.state));
        if (do_calculate) {
            float score = window.images.first.pyrCompare(window.images.second, window.state);
            if (score < bestScore) {
                bestScore = score;
                printf("error %g\n", score);
                //std::cout << glob_balance << std::endl;
                //std::cout << glob_brightness << std::endl;
                printf("suggested next step (rot x, y, z, translate x, y, z):\n");
                std::cout << window.images.first.d_pyrCompare(window.images.second, window.state).t() << std::endl;
            }
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
            float scale = -0.3 * std::min(shape.width, shape.height) / float(std::min(window.size.height, window.size.width));
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
    if (argc > 4) {
        Matrix depthmap;
        cv::imread(argv[4], cv::IMREAD_GRAYSCALE | cv::IMREAD_ANYDEPTH).convertTo(depthmap, CV_32FC1, (argc > 5) ? atof(argv[5]) : 1);
        if (not depthmap.empty()) {
            images.first.depth = depthmap;
            printf("loaded depthmap: %ix%i\n", depthmap.rows, depthmap.cols);
        }
    }
    images.first = images.first.downscale();
    images.second = images.second.downscale();
    Window<int> reference("reference", images, depthPaint);
    reference.show(images.first);
    Window<ProjectionMatrix> view("view", images, viewUpdate<false>);
    view.state = ProjectionMatrix((argc > 3) ? atof(argv[3]) : 30.f);
    view.show(images.second);
    char c;
    auto momentum = Matf<paramCount, 1>::zeros();
    float score = 1e10;
    while (1) {
        switch (char(cv::waitKey())) {
            static bool h{false}, d{false};
            case ' ':
                do_calculate ^= true;
                viewUpdate<true>(view);
                break;
            case 'd':
                if (d ^= true) {
                    view.show(images.second);
                } else {
                    view.show(render(images, view.state));
                } break;
            case 'h':
                if (h ^= true) {
                    reference.show(images.first.depth);
                } else {
                    reference.show(images.first);
                } break;
            case 's': {
                for (int i=0; i<5; i++) {
                    auto step = images.first.d_pyrCompare(images.second, view.state);
                    momentum = 0.8 * momentum - 0.2 * step;
                    Matrix transformation = parseEuclidean(momentum);
                    view.state.apply(transformation);
                    float newScore = images.first.pyrCompare(images.second, view.state);
                    if (newScore > score) {
                        printf("reset (%g > %g) ", newScore, score);
                        momentum = -0.1 * step;
                        view.state.revert();
                        transformation = parseEuclidean(momentum);
                        view.state.apply(transformation);
                    }
                    //std::cout << "params = " << step.t() << std::endl;
                }
                score = images.first.pyrCompare(images.second, view.state);
                printf("score: %g\n", score);
                viewUpdate<true>(view);
                } break;
            case 27:
                return 0;
        }
    }
}
