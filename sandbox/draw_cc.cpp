//draw_cc: kreslí křivku s po částech konstatní křivostí, a bod, který po té křivce jede
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>
#include <deque>
#include <tuple>
#include <chrono>
using cv::Point;
using cv::Vec2f;
using std::deque;

inline Vec2f to_vector(Point p)
{
    return Vec2f{float(p.x), float(p.y)};
}

float clamp(float v, float low, float high)
{
    return v < low ? low : high < v ? high : v;
}

void limit_radius(float &r, float d, float vx, float vy)
{
    float low = -d / (1 + vy), high = d / (1 - vy);
    r = clamp(r, low, high);
}

void make_plan(deque<Point> &curve, int width, int height)
{
    unsigned max_steps = 1000;
    float max_curvature = 1e-2;
    static auto rng = cv::theRNG();
    Vec2f p, v;
    if (curve.size() >= 5) {
        p[0] = clamp(curve.back().x, 1, width - 2);
        p[1] = clamp(curve.back().y, 1, height - 2);
        v = p - to_vector(curve[curve.size() - 5]);
        if (cv::norm(v) == 0) {
            v = Vec2f(2 * rng(width / 2) + 1 - (int(p[0]) % 2), rng(height)) - p;
        }
    } else {
        p = Vec2f(rng(width), rng(height));
        v = Vec2f(rng.uniform(-1.f, 1.f), rng.uniform(-1.f, 1.f));
    }
    v /= cv::norm(v);
    float nacc = rng.uniform(-max_curvature, max_curvature);
    float r = 1 / nacc;
    limit_radius(r, width - p[0] - 1, -v[0], -v[1]);
    limit_radius(r, p[1], v[1], -v[0]);
    limit_radius(r, p[0], v[0], v[1]);
    limit_radius(r, height - p[1] - 1, -v[1], v[0]);
    nacc = 1 / r;
    if (not (std::abs(nacc) < max_curvature)) {
        nacc = 0;
        v = Vec2f(2 * rng(width / 2) + 1 - (int(p[0]) % 2), rng(height)) - p;
        max_steps = cv::norm(v);
        v /= cv::norm(v);
    }
    int steps = (nacc != 0) ? std::min(rng(4 * std::abs(r)), max_steps) : rng(max_steps);
    printf("starting with %lu, ", curve.size());
    for (int j=0; j<steps; ++j) {
        curve.push_back(Point(p[0], p[1]));
        Vec2f n(-v[1], v[0]);
        v -= n * nacc;
        v /= cv::norm(v);
        p += v;
    }
    printf("ended with %lu.\n", curve.size());
}

int main()
{
    using Clock = std::chrono::steady_clock;
    const cv::Scalar black(0, 0, 0), gray(128, 128, 128), white(255, 255, 255);
    int width = 1650, height = 1000;
    int speed = 800;
    const char *winname = "constant curvature";
    size_t lookahead = 500;
    std::vector<cv::Scalar> rainbow, fadeout;
    for (int i=0; i<=lookahead; ++i) {
        fadeout.push_back(white * std::sqrt(float(i) / lookahead));
    }
    for (int i=0; i<64; ++i) {
        rainbow.emplace_back(8 * std::abs(i - 32), 0, 255 - 8 * std::abs(i - 32));
    }
    cv::Mat canvas(height, width, CV_8UC3);
    canvas = white;
    deque<Point> curve;
    static auto rng = cv::theRNG();
    auto time_prev = Clock::now();
    for (int i=0; ; ++i) {
        while (curve.size() < lookahead) {
            make_plan(curve, width, height);
        }
        cv::circle(canvas, curve.front(), 10, rainbow[i % rainbow.size()], -1);
        cv::circle(canvas, curve.front(), 3, white, -1);
        for (int j=lookahead; j >= 0; --j) {
            cv::line(canvas, curve[j], curve[j + 1], fadeout[j]);
        }
        cv::imshow(winname, canvas);
        cv::circle(canvas, curve.front(), 51, white, -1);
        if (char(cv::waitKey(20)) == 27) {
            break;
        }
        auto time_now = Clock::now();
        float grabbing_time = std::chrono::duration_cast<std::chrono::duration<float>>(time_now - time_prev).count();
        curve.erase(curve.begin(), curve.begin() + clamp(size_t(grabbing_time * speed), 1, curve.size() - 5));
        time_prev = time_now;
    }
    return 0;
}
