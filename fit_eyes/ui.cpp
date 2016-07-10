#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include <vector>
#include <iostream>
const char winname[] = "fit eyes";

namespace {
void onMouse(int, int, int, int, void*);

struct State
{
    using Drag = std::pair<Pixel, Pixel>;
    const char *winname;
    const Bitmap3 &canvas;
    vector<Drag> record;
    const std::array<bool, 5> is_rectangle{{true, true, true, false, false}};
    bool pressed = false;
    State(const char *winname, const Bitmap3 &img) : winname{winname}, canvas{img} {
        cv::namedWindow(winname);
        cv::setMouseCallback(winname, onMouse, this);
        cv::imshow(winname, canvas);
    }
    ~State() {
        cv::setMouseCallback(winname, NULL, NULL);
    }
    void render() const {
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
    Eye eye(int index) const {
        assert (not is_rectangle.at(index));
        return Eye(to_vector(record[index].first), cv::norm(record[index].second - record[index].first));
    }
    Region region(int index) const {
        assert (is_rectangle.at(index));
        return Region{record[index].first, record[index].second};
    }
    bool done() const {
        return not pressed and record.size() >= is_rectangle.size();
    }
};

void onMouse(int event, int x, int y, int, void* param)
{
    Pixel pos(x, y);
    State &state = *static_cast<State*>(param);
    if (event == cv::EVENT_LBUTTONDOWN) {
        state.pressed = true;
        state.record.emplace_back(std::make_pair(pos, pos));
    } else if (event == cv::EVENT_MOUSEMOVE and state.pressed) {
        state.record.back().second = pos;
        state.render();
    } else if (event == cv::EVENT_LBUTTONUP and state.pressed) {
        state.pressed = false;
        state.record.back().second = pos;
        state.render();
    }
}

}

Face mark_eyes(Bitmap3 &img)
{
    State state(winname, img);
    do {
        if (char(cv::waitKey(100)) == 27) {
            exit(0);
        }
    } while (not state.done());
    Face result{img, state.region(0), state.region(1), state.region(2), state.eye(3), state.eye(4)};
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

void Face::render(const Bitmap3 &image) const
{
    Bitmap3 result = image.clone();
    render_region(main_region, main_tsf, result);
    render_region(eye_region, eye_tsf, result);
    render_region(nose_region, nose_tsf, result);
    for (const Eye &eye : eyes) {
        cv::circle(result, to_pixel(eye_tsf(eye.pos)), eye.radius, cv::Scalar(0.5, 1.0, 0));
    }
    const float font_size = 1;
    for (int i=0; i<2; i++) {
        cv::putText(result, std::to_string(eyes[i].pos[0]), Pixel(0, font_size * (24 * i + 12)), cv::FONT_HERSHEY_PLAIN, font_size, cv::Scalar(0.2, 0.5, 1), font_size + 1);
        cv::putText(result, std::to_string(eyes[i].pos[1]), Pixel(20, font_size * (24 * i + 24)), cv::FONT_HERSHEY_PLAIN, font_size, cv::Scalar(0.3, 1, 0.3), font_size + 1);
    }
    cv::imshow(winname, result);
}

Gaze calibrate(Face &face, VideoCapture &cap, Pixel window_size)
{
    const int divisions = 3;
    const int necessary_support = 2 * divisions * divisions;
    const char winname[] = "calibrate";
    const Vector3 bgcolor(0.7, 0.6, 0.5);
    vector<Measurement> measurements;
    vector<std::pair<Bitmap3, Transformation>> memory;
    Bitmap3 canvas(window_size.y, window_size.x);
    canvas = bgcolor;
    cv::namedWindow(winname);
    cv::imshow(winname, canvas);
    cv::waitKey(5000);
    Vector2 cell(window_size.x / divisions, window_size.y / divisions);
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_real_distribution<> urand(0, 1);
    std::pair<Bitmap3, Vector2> task;
    while (1) {
        vector<Vector2> points;
        for (int i=0; i<divisions; i++) {
            for (int j=0; j<divisions; j++) {
                points.emplace_back(cell[0] * (j + urand(generator)), cell[1] * (i + urand(generator)));
            }
        }
        std::shuffle(points.begin(), points.end(), generator);
        for (Vector2 point : points) {
            canvas = bgcolor;
            cv::circle(canvas, to_pixel(point), 50, cv::Scalar(0, 0.6, 1), -1);
            cv::circle(canvas, to_pixel(point), 5, cv::Scalar(0, 0, 0.3), -1);
            cv::imshow(winname, canvas);
            if (not task.first.empty()) {
                face.refit(task.first);
                face.render(task.first);
                measurements.emplace_back(std::make_pair(face(), task.second));
            }
            bool do_recalc_gaze = measurements.size() % 9 == 0 and measurements.size() >= necessary_support;
            if (do_recalc_gaze) {
                int support = necessary_support;
                const float precision = 150;
                std::cout << "starting to solve..." << std::endl;
                Gaze result(measurements, support, precision);
                    for (auto pair : measurements) {
                        std::cout << pair.first << " -> " << result(pair.first) << " vs. " << pair.second << ((cv::norm(result(pair.first) - pair.second) < precision) ? " INLIER" : " out") << std::endl;
                    }
                if (support >= necessary_support) {
                    cv::destroyWindow(winname);
                    return result;
                }
            }
            cv::waitKey(400);
            task.first.read(cap);
            task.second = point;
        }
    }
}
