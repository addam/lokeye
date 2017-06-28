//draw_cc: kreslí křivku s po částech konstatní křivostí, a bod, který po té křivce jede
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>
#include <tuple>
using cv::Point;
using cv::Vec2f;
using std::vector;

float clamp(float v, float low, float high)
{
    return v < low ? low : high < v ? high : v;
}

void limit_radius(float &r, float d, float vx, float vy)
{
    float low = -d / (1 + vy), high = d / (1 - vy);
    r = clamp(r, low, high);
}

vector<Point> make_plan(int width, int height, unsigned count)
{
    std::vector<Point> result;
    unsigned max_steps = 1000;
    float max_curvature = 1e-2;
    static auto rng = cv::theRNG();
    Vec2f p(rng(width), rng(height));
    Vec2f v(rng.uniform(-1.f, 1.f), rng.uniform(-1.f, 1.f));
    v /= cv::norm(v);
    while (1) {
        float nacc = rng.uniform(-max_curvature, max_curvature);
        float r = 1 / nacc;
        limit_radius(r, width - p[0] - 1, -v[0], -v[1]);
        limit_radius(r, p[1], v[1], -v[0]);
        limit_radius(r, p[0], v[0], v[1]);
        limit_radius(r, height - p[1] - 1, -v[1], v[0]);
        nacc = 1 / r;
        //printf("from %g, %g planned radius %g = curvature %g\n", p[0], p[1], r, nacc);
        if (not (std::abs(nacc) < max_curvature)) {
            //printf("not std::abs(%g) = %g < %g\n", nacc, std::abs(nacc), max_curvature);
            nacc = 0;
            v = Vec2f(width / 2 - p[0], height / 2 - p[1]);
            v /= cv::norm(v);
        }
        int steps = std::min(rng(max_steps * std::abs(r)), max_steps);
        for (int j=0; j<steps; ++j) {
            result.emplace_back(Point(p[0], p[1]));
            if (result.size() >= count) {
                return result;
            }
            Vec2f n(-v[1], v[0]);
            v -= n * nacc;
            v /= cv::norm(v);
            p += v;
        }
        p[0] = clamp(p[0], 1, width - 2);
        p[1] = clamp(p[1], 1, height - 2);
    }
}

int main()
{
    const cv::Scalar black(0, 0, 0), gray(128, 128, 128), white(255, 255, 255);
    int width = 1650, height = 1000;
    const char *winname = "constant curvature";
    size_t lookahead = 500;
    std::vector<cv::Scalar> rainbow;
    for (int i=0; i<=lookahead; ++i) {
        rainbow.push_back(white * std::sqrt(float(i) / lookahead));
    }
    cv::Mat canvas(height, width, CV_8UC3);
    canvas = white;
    auto curve = make_plan(width, height, 20000);
    for (size_t i=0; i<curve.size(); i += 15) {
        cv::circle(canvas, curve[i], 50, black, -1);
        cv::circle(canvas, curve[i], 5, white, -1);
        for (int j=lookahead; j >= 0; --j) {
            if (i + j + 1 < curve.size()) {
                cv::line(canvas, curve[i + j], curve[i + j + 1], rainbow[j]);
            }
        }
        cv::imshow(winname, canvas);
        cv::circle(canvas, curve[i], 51, white, -1);
        if (char(cv::waitKey(20)) == 27) {
            break;
        }
    }
    return 0;
}
