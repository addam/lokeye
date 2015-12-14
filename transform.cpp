//#define WITH_OPENMP
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/video.hpp"
using cv::Point;
using Vector = cv::Vec3f;
using Mat = cv::Mat_<float>;
using Image = cv::Mat_<Vector>;

const char *wint = "transform", *winr = "ref", *wind = "depth";

Image ref;
Image view;
Mat cam, rot, trans;
Mat depth;

inline float pow2(float x)
{
    return x*x;
}

Point convert(Vector v, const cv::Size &size)
{
    float scale = std::min(size.width, size.height) / 2.f;
    Point center{size.width/2, size.height/2};
    return Point{int(v[0] * scale), int(v[1] * scale)} + center;
}

Vector convert(Point p, float z, const cv::Size &size)
{
    float scale = std::min(size.width, size.height) / 2.f;
    Point center{size.width/2, size.height/2};
    p -= center;
    return Vector{p.x / scale, p.y / scale, z};
}

float difference(Vector a, Vector b)
{
    return std::abs(a[0] - b[0]) + std::abs(a[1] - b[1]) + std::abs(a[2] - b[2]);
}

Image transform()
{
    static float best_total_error = 1e10;
    float total_error = 0;
    Mat projection = cam * trans * rot;
    Image result = view.clone();
    Mat zbuffer = Mat::ones(result.size()) * 1e8;
    Mat errmap = Mat::zeros(result.size());
    cv::Rect screen{Point{0, 0}, result.size()};
    for (int i=0; i<ref.rows; i++) {
        for (int j=0; j<ref.cols; j++) {
            float z = depth(i, j);
            if (z > 0) {
                Vector v = convert(Point{j, i}, z, ref.size());
                std::vector<Vector> tmp = {v};
                cv::perspectiveTransform(tmp, tmp, projection);
                Point p = convert(tmp[0], ref.size());
                if (screen.contains(p)) {
                    float d = difference(result(p), ref(i, j));
                    errmap(i, j) = d / 3;
                    total_error += d;
                    if (zbuffer(p) > tmp[0][2]) {
                        result(p) = ref(i, j);
                        zbuffer(p) = tmp[0][2];
                    }
                } else {
                    total_error += 3;
                }
            }
        }
    }
    cv::imshow(wind, errmap);
    if (total_error > 0 and total_error < best_total_error) {
        printf("error %f\n", total_error);
        best_total_error = total_error;
    }
    std::vector<Vector> axes{{0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}};
    cv::perspectiveTransform(axes, axes, projection);
    Point origin{int(axes[0][0]), int(axes[0][1])};
    for (int i=1; i<axes.size(); i++) {
        cv::line(result, convert(axes[0], result.size()), convert(axes[i], result.size()), Vector{float(i==1), float(i==2), float(i==3)});
    }
    return result;
}

void redraw()
{
    Image img = transform();
    imshow(wint, img);
}

static void viewEvent(int event, int x, int y, int flags, void* ptr)
{
    static bool pressed{false};
    static int prev_x, prev_y;
	if (event == cv::EVENT_LBUTTONUP) {
        pressed = false;
    } else if (event == cv::EVENT_LBUTTONDOWN) {
        prev_x = x;
        prev_y = y;
        pressed = true;
    }
    if (pressed) {
        float dx = 2.f * (x - prev_x) / (ref.cols);
        float dy = 2.f * (y - prev_y) / (ref.rows);
        if (flags & cv::EVENT_FLAG_SHIFTKEY) {
            trans(0, 3) -= dx;
            trans(1, 3) -= dy;
        } else if (flags & cv::EVENT_FLAG_CTRLKEY) {
            trans(2, 3) += dy;
            Mat r = Mat::eye(4, 4);
            r(1, 0) = -(r(0, 1) = dx);
            r(0, 0) = r(1, 1) = std::sqrt(1 - pow2(dx));
            rot = r * rot;
        } else {
            Mat rx = Mat::eye(4, 4), ry = Mat::eye(4, 4);
            rx(0, 2) = -(rx(2, 0) = dx);
            rx(0, 0) = rx(2, 2) = std::sqrt(1 - pow2(dx));
            ry(1, 2) = -(ry(2, 1) = dy);
            ry(1, 1) = ry(2, 2) = std::sqrt(1 - pow2(dy));
            rot = rx * ry * rot;
        }
        prev_x = x;
        prev_y = y;
        redraw();
    }
}

static void drawEvent(int event, int x, int y, int flags, void* ptr)
{
    static bool pressed{false};
    static float start_x, start_y;
	if (event == cv::EVENT_LBUTTONDOWN) {
        if (flags & cv::EVENT_FLAG_CTRLKEY) {
            start_x = x;
            start_y = y;
            pressed = true;
        }
    } else if (event == cv::EVENT_LBUTTONUP) {
        if (pressed) {
            int sx = start_x, sy = start_y;
            int width = std::abs(sx - x), height = std::abs(sy - y);
            int left = std::max(0, sx - width), top = std::max(0, sy - height), right = std::min(depth.cols - 1, sx + width), bottom = std::min(depth.rows - 1, sy + height);
            float scale = 2.f * std::min(width, height) / std::min(depth.rows, depth.cols);
            for (int i=top; i < bottom; i++) {
                for (int j=left; j < right; j++) {
                    float radius = pow2(float(sx - j) / width) + pow2(float(sy - i) / height);
                    if (radius < 1) {
                        depth(i, j) += scale * std::sqrt(1 - radius);
                    }
                }
            }
            cv::imshow(winr, ref);
            cv::imshow(wind, depth);
            pressed = false;
        } else {
        }
    } else if (pressed) {
        Image tmp = ref.clone();
        cv::ellipse(tmp, cv::RotatedRect{cv::Point2f{start_x, start_y}, cv::Point2f{2.f*std::abs(float(x) - start_x), 2.f*std::abs(float(y) - start_y)}, 0.f}, cv::Scalar{0.f, 0.f, 1.f});
        cv::imshow(winr, tmp);
    } else if (flags & cv::EVENT_FLAG_ALTKEY) {
    } else {
    }
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: transform (reference.jpg) (view.jpg)\n");
        return 1;
    }
    cam = Mat::eye(4, 4);
    cam(3, 2) = 35.f / ((argc > 3) ? atof(argv[3]) : 30.f);
    rot = -1 * Mat::eye(4, 4);
    trans = Mat::eye(4, 4);
    trans(2, 3) = -2;
    cv::imread(argv[1], cv::IMREAD_COLOR | cv::IMREAD_ANYDEPTH).convertTo(ref, CV_32FC3, 1./256);
    cv::imread(argv[2], cv::IMREAD_COLOR | cv::IMREAD_ANYDEPTH).convertTo(view, CV_32FC3, 1./256);
    depth = Mat::zeros(ref.size());
    
    cv::namedWindow(winr, 1);
    cv::setMouseCallback(winr, drawEvent, 0);
    imshow(winr, ref);
    cv::namedWindow(wint, 2);
    cv::setMouseCallback(wint, viewEvent, 0);
    redraw();
    cv::namedWindow(wind, 3);
    imshow(wind, depth);
    while (char(cv::waitKey()) != 27);
    return 0;
}
