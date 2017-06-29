//draw_cc: kreslí křivku s po částech konstatní křivostí, a bod, který po té křivce jede
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>
#include <tuple>
using cv::Point;
using cv::Vec2f;
using std::vector;

Vec2f p, v;
size_t begin, end;

float clamp(float v, float low, float high)
{
    return v < low ? low : high < v ? high : v;
}

void limit_radius(float &r, float d, float vx, float vy)
{
    float low = -d / (1 + vy), high = d / (1 - vy);
    r = clamp(r, low, high);
}

void make_plan(vector<Point> &result, int width, int height)
{
    unsigned max_steps = 1000;
    float max_curvature = 1e-2;
    static auto rng = cv::theRNG();
    float nacc = rng.uniform(-max_curvature, max_curvature);
    float r = 1 / nacc;
    limit_radius(r, width - p[0] - 1, -v[0], -v[1]);
    limit_radius(r, p[1], v[1], -v[0]);
    limit_radius(r, p[0], v[0], v[1]);
    limit_radius(r, height - p[1] - 1, -v[1], v[0]);
    nacc = 1 / r;
    if (not (std::abs(nacc) < max_curvature)) {
        nacc = 0;
        v = Vec2f(width / 2 - p[0], height / 2 - p[1]);
        v /= cv::norm(v);
    }
    int steps = std::min(rng(4 * std::abs(r)), max_steps);
    for (int j=0; j<steps; ++j) {
        result[end] = Point(p[0], p[1]);
        end = (end + 1) % result.size();
        Vec2f n(-v[1], v[0]);
        v -= n * nacc;
        v /= cv::norm(v);
        p += v;
    }
    p[0] = clamp(p[0], 1, width - 2);
    p[1] = clamp(p[1], 1, height - 2);
}

int main()
{
    const cv::Scalar black(0, 0, 0), gray(128, 128, 128), white(255, 255, 255);
    int width = 1650, height = 1000;
    const char *winname = "constant curvature";
    size_t lookahead = 500;
    std::vector<cv::Scalar> rainbow, fadeout;
    for (int i=0; i<=lookahead; ++i) {
        fadeout.push_back(white * std::sqrt(float(i) / lookahead));
    }
    for (int i=0; i<64; ++i) {
        rainbow.emplace_back(0, 8 * std::abs(i - 32), 255 - 8 * std::abs(i - 32));
    }
    cv::Mat canvas(height, width, CV_8UC3);
    canvas = white;
    vector<Point> curve(2000);
    static auto rng = cv::theRNG();
    p = Vec2f(rng(width), rng(height));
    v = Vec2f(rng.uniform(-1.f, 1.f), rng.uniform(-1.f, 1.f));
    v /= cv::norm(v);
    begin = end = 0;
    while (1) {
        while ((end - begin + curve.size()) % curve.size() < lookahead) {
            make_plan(curve, width, height);
        }
        cv::circle(canvas, curve[begin], 10, rainbow[(begin / 15) % rainbow.size()], -1);
        cv::circle(canvas, curve[begin], 3, white, -1);
        for (int j=lookahead; j >= 0; --j) {
            cv::line(canvas, curve[(begin + j) % curve.size()], curve[(begin + j + 1) % curve.size()], fadeout[j]);
        }
        cv::imshow(winname, canvas);
        cv::circle(canvas, curve[begin], 51, white, -1);
        if (char(cv::waitKey(20)) == 27) {
            break;
        }
        begin = (begin + 15) % curve.size();
    }
    return 0;
}
