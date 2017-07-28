#include <vector>
#include <iostream>
#include <random>
#include <deque>
#include <chrono>
#include "main.h"
#include "bitmap.h"
#include "optimization.h"

namespace {

class Initialization
{
    using Drag = std::pair<Pixel, Pixel>;
    const char *winname;
    const Bitmap3 &canvas;
    vector<Drag> record;
    const std::array<bool, 3> is_rectangle = {true, false, false};
    bool pressed = false;
public:
    Initialization(const char*, const Bitmap3&);
    ~Initialization();
    void render() const;
    Circle eye(int index) const;
    Region region(int index) const;
    bool done() const;
    static void onMouse(int, int, int, int, void*);
};

class Calibration
{
    using Clock = std::chrono::steady_clock;
    const char *winname;
    const cv::Scalar white = cv::Scalar(255, 255, 255);
    vector<Measurement> measurements;
    const int width = 1650, height = 1000;
    const int speed = 800;
    const int lookahead = 500;
    std::vector<cv::Scalar> rainbow, fadeout;
    cv::Mat canvas;
    std::deque<Pixel> curve;
    Clock::time_point time_prev;
    
    /// Extend the curve by a random arc
    void extend();

    /** Bound the signed circle radius so that it does not cross a wall
     * @param[in,out] r  The signed radius (positive means counterclockwise motion)
     * @param d Distance from the left border
     * @param vx Tangent x
     * @param vy Tangent y
     */
    static void limit_radius(float &r, float d, float vx, float vy);
public:
    bool running = false;
    Calibration(const char*);
    ~Calibration();
    bool render();
    
    /// Get current gaze target
    Vector2 operator() () const;
};

Initialization::Initialization(const char *winname, const Bitmap3 &img) : winname{winname}, canvas{img}
{
    cv::namedWindow(winname);
    cv::setMouseCallback(winname, onMouse, this);
    cv::imshow(winname, canvas);
}

Initialization::~Initialization()
{
    cv::destroyWindow(winname);
}

void Initialization::render() const
{
    Bitmap3 result = canvas.clone();
    for (int i=0; i<record.size(); ++i) {
        const Drag &d = record[i];
        if (is_rectangle[i]) {
            cv::rectangle(result, d.first, d.second, cv::Scalar(0, 1.0, 1.0));
        } else {
            cv::circle(result, d.first, cv::norm(d.second - d.first), cv::Scalar(0.5, 1.0, 0));
        }
    }
    cv::imshow(winname, result);
}

Circle Initialization::eye(int index) const
{
    assert (not is_rectangle.at(index));
    return {to_vector(record[index].first), cv::norm(record[index].second - record[index].first)};
}

Region Initialization::region(int index) const
{
    assert (is_rectangle.at(index));
    return Region{record[index].first, record[index].second};
}

bool Initialization::done() const
{
    return not pressed and record.size() >= is_rectangle.size();
}

void Initialization::onMouse(int event, int x, int y, int, void* param)
{
    Pixel pos(x, y);
    Initialization &session = *static_cast<Initialization*>(param);
    if (event == cv::EVENT_LBUTTONDOWN) {
        session.pressed = true;
        session.record.emplace_back(std::make_pair(pos, pos));
    } else if (event == cv::EVENT_MOUSEMOVE and session.pressed) {
        session.record.back().second = pos;
        session.render();
    } else if (event == cv::EVENT_LBUTTONUP and session.pressed) {
        session.pressed = false;
        session.record.back().second = pos;
        session.render();
    }
}

Calibration::Calibration(const char *winname) : winname{winname}
{
    for (int i=0; i<=lookahead; ++i) {
        fadeout.push_back(white * std::sqrt(float(i) / lookahead));
    }
    for (int i=0; i<64; ++i) {
        rainbow.emplace_back(8 * std::abs(i - 32), 0, 255 - 8 * std::abs(i - 32));
    }
    canvas = cv::Mat(height, width, CV_8UC3, white);
    time_prev = Clock::now();
}

Calibration::~Calibration()
{
    cv::destroyWindow(winname);
}

void Calibration::limit_radius(float &r, float d, float vx, float vy)
{
    float low = -d / (1 + vy), high = d / (1 - vy);
    r = clamp(r, low, high);
}

void Calibration::extend()
{
    unsigned max_steps = 1000;
    float max_curvature = 1e-2;
    static auto rng = cv::theRNG();
    Vector2 p, v;
    if (curve.size() >= 5) {
        p[0] = clamp(curve.back().x, 1, width - 2);
        p[1] = clamp(curve.back().y, 1, height - 2);
        v = p - to_vector(curve[curve.size() - 5]);
        if (cv::norm(v) == 0) {
            v = Vector2(2 * rng(width / 2) + 1 - (int(p[0]) % 2), rng(height)) - p;
        }
    } else {
        p = Vector2(rng(width), rng(height));
        v = Vector2(rng.uniform(-1.f, 1.f), rng.uniform(-1.f, 1.f));
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
        v = Vector2(2 * rng(width / 2) + 1 - (int(p[0]) % 2), rng(height)) - p;
        max_steps = cv::norm(v);
        v /= cv::norm(v);
    }
    int steps = (nacc != 0) ? std::min(rng(4 * std::abs(r)), max_steps) : rng(max_steps);
    for (int j=0; j<steps; ++j) {
        curve.push_back(to_pixel(p));
        Vector2 n(-v[1], v[0]);
        v -= n * nacc;
        v /= cv::norm(v);
        p += v;
    }
}

bool Calibration::render()
{
    auto time_now = Clock::now();
    float elapsed_time = std::chrono::duration_cast<std::chrono::duration<float>>(time_now - time_prev).count();
    size_t skipped_pixels = running ? 1 + speed * elapsed_time : 0;
    while (curve.size() < lookahead + skipped_pixels) {
        extend();
    }
    for (int j=1; j < skipped_pixels; ++j) {
        cv::line(canvas, curve[j - 1], curve[j], white);
    }
    curve.erase(curve.begin(), curve.begin() + skipped_pixels);
    static int i=0;
    cv::circle(canvas, curve.front(), 10, rainbow[i++ % rainbow.size()], -1);
    cv::circle(canvas, curve.front(), 3, white, -1);
    for (int j=lookahead; j >= 0; --j) {
        cv::line(canvas, curve[j], curve[j + 1], fadeout[j]);
    }
    cv::imshow(winname, canvas);
    cv::circle(canvas, curve.front(), 51, white, -1);
    if (char(cv::waitKey(20)) == 27) {
        return false;
    }
    running = true;
    time_prev = time_now;
    return true;
}

Vector2 Calibration::operator() () const
{
    return to_vector(curve.front());
}

}

Face init_interactive(const Bitmap3 &img)
{
    Initialization session("mark face", img);
    do {
        if (char(cv::waitKey(100)) == 27) {
            throw NoFaceException();
        }
    } while (not session.done());
    Face result{img, session.region(0), session.eye(1), session.eye(2)};
    return result;
}

void render_region(Region region, Transformation tsf, Bitmap3 &canvas)
{
    std::array<Vector2, 4> vertices{to_vector(region.tl()), Vector2(region.x, region.y + region.height), to_vector(region.br()), Vector2(region.x + region.width, region.y)};
    std::for_each(vertices.begin(), vertices.end(), [tsf](Vector2 &v) { v = tsf(v); });
    for (int i = 0; i < 4; i++) {
        cv::line(canvas, to_pixel(vertices[i]), to_pixel(vertices[(i+1)%4]), cv::Scalar(0, 1.0, 1.0));
    }
}

void Face::render(const Bitmap3 &image, const char *winname) const
{
    Bitmap3 result = image.clone();
    render_region(main_region, main_tsf, result);
    children.render(result);
    for (const Circle &eye : fitted_eyes) {
        if (result.contains(main_tsf(eye.center))) {
            cv::circle(result, to_pixel(main_tsf(eye.center)), main_tsf.scale(eye.center) * eye.radius, cv::Scalar(0.5, 1.0, 0));
        }
    }
    const float font_size = 1;
    for (int i=0; i<2; i++) {
        cv::putText(result, std::to_string(eyes[i].center[0]), Pixel(0, font_size * (24 * i + 12)), cv::FONT_HERSHEY_PLAIN, font_size, cv::Scalar(0.2, 0.5, 1), font_size + 1);
        cv::putText(result, std::to_string(eyes[i].center[1]), Pixel(20, font_size * (24 * i + 24)), cv::FONT_HERSHEY_PLAIN, font_size, cv::Scalar(0.3, 1, 0.3), font_size + 1);
    }
    cv::imshow(winname, result);
}

Gaze calibrate_interactive(Face &face, VideoCapture &cap, Pixel window_size)
{
    const int necessary_support = 20;
    Calibration session("calibration");
    Bitmap3 image;
    std::vector<Measurement> measurements;
    for (int i=0; ; ++i) {
        session.render();
        image.read(cap, true);
        face.refit(image);
        measurements.emplace_back(std::make_pair(face(), session()));
        bool do_recalc_gaze = measurements.size() % 9 == 0 and measurements.size() >= necessary_support;
        if (do_recalc_gaze) {
            session.running = false;
            int support = necessary_support;
            const float precision = 150;
            std::cout << "starting to solve..." << std::endl;
            Gaze result(measurements, support, precision);
            for (auto pair : measurements) {
                std::cout << pair.first << " -> " << result(pair.first) << " vs. " << pair.second << ((cv::norm(result(pair.first) - pair.second) < precision) ? " INLIER" : " out") << std::endl;
            }
            if (support >= necessary_support) {
                return result;
            }
        }
    }
}
