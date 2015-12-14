//#define WITH_OPENMP
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/video.hpp"
using cv::Point;
using Vector = cv::Vec3f;
using Mat = cv::Mat_<float>;
using Image = cv::Mat_<Vector>;

const char* winname = "depth";

Image ref;
Mat rot;
Mat trans;

inline float pow2(float x)
{
    return x*x;
}

Point upscale(Vector v, const Image &img)
{
    float size = std::min(img.cols, img.rows) / 2.f;
    Point center{img.cols/2, img.rows/2};
    return Point{int(v[0] * size), int(v[1] * size)} + center;
}

Image transform()
{
    Mat cam = Mat::eye(4, 4);
    cam(3, 2) = 1;
    Image result = Image::zeros(ref.size());
    std::vector<Vector> axes{{0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}};
    cv::perspectiveTransform(axes, axes, cam * trans * rot);
    Point origin{int(axes[0][0]), int(axes[0][1])};
    for (int i=1; i<axes.size(); i++) {
        cv::line(result, upscale(axes[0], result), upscale(axes[i], result), Vector{float(i==1), float(i==2), float(i==3)});
    }
    return result;
}

void redraw()
{
    Image img = transform();
    imshow(winname, img);
}

static void onMouse(int event, int x, int y, int flags, void* ptr)
{
    static bool pressed{false};
    static int prev_x, prev_y;
	if (event == cv::EVENT_LBUTTONDOWN) {
        prev_x = x;
        prev_y = y;
        pressed = true;
    } else if (event == cv::EVENT_LBUTTONUP) {
        pressed = false;
    }
    if (pressed) {
        float dx = 2.f * (x - prev_x) / (ref.cols);
        float dy = 2.f * (y - prev_y) / (ref.rows);
        if (flags & cv::EVENT_FLAG_SHIFTKEY) {
            trans(0, 3) += dx;
            trans(1, 3) += dy;
        } else if (flags & cv::EVENT_FLAG_CTRLKEY) {
            trans(2, 3) += dy;
            Mat r = Mat::eye(4, 4);
            r(1, 0) = -(r(0, 1) = dx);
            r(0, 0) = r(1, 1) = std::sqrt(1 - pow2(dx));
            rot = r * rot;
        } else {
            Mat rx = Mat::eye(4, 4), ry = Mat::eye(4, 4);
            rx(2, 0) = -(rx(0, 2) = dx);
            rx(0, 0) = rx(2, 2) = std::sqrt(1 - pow2(dx));
            ry(2, 1) = -(ry(1, 2) = dy);
            ry(1, 1) = ry(2, 2) = std::sqrt(1 - pow2(dy));
            rot = rx * ry * rot;
        }
        prev_x = x;
        prev_y = y;
        redraw();
    }
}

int main(int argc, char** argv)
{
    rot = Mat::eye(4, 4);
    trans = Mat::eye(4, 4);
    trans(2, 3) = 2;
    cv::namedWindow(winname, 1);
    cv::setMouseCallback(winname, onMouse, 0);
    ref = Image::zeros(300, 400);//cv::imread(argv[1], cv::IMREAD_GRAYSCALE | cv::IMREAD_ANYDEPTH);
    imshow("original", ref);
    redraw();
    while (char(cv::waitKey()) != 27);
    return 0;
}
