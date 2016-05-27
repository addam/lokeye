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
    const char *winname;
    const Bitmap3 &canvas;
    Pixel begin, end;
    std::vector<Eye> eyes;
    bool has_rectangle = false;
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
        if (has_rectangle) {
            cv::rectangle(result, begin, end, cv::Scalar(0, 1.0, 1.0));
        }
        for (const Eye &eye : eyes) {
            cv::circle(result, to_pixel(eye.pos), eye.radius, cv::Scalar(0.5, 1.0, 0));
        }
        cv::imshow(winname, result);
    }
    bool done() const {
        return has_rectangle and eyes.size() >= 2 and not pressed;
    }
};

void onMouse(int event, int x, int y, int, void* param)
{
    Pixel pos(x, y);
    State &state = *static_cast<State*>(param);
    if (event == cv::EVENT_LBUTTONDOWN) {
        state.pressed = true;
    } else if (event == cv::EVENT_LBUTTONUP) {
        state.pressed = false;
    }
    if (event == cv::EVENT_LBUTTONDOWN and not state.has_rectangle) {
        state.begin = pos;
    } else if (event == cv::EVENT_LBUTTONDOWN and state.has_rectangle) {
        state.eyes.emplace_back(Eye{to_vector(pos), 1.0f});
    } else if (event == cv::EVENT_MOUSEMOVE and state.pressed and state.eyes.empty()) {
        state.end = pos;
        state.has_rectangle = true;
    } else if (event == cv::EVENT_MOUSEMOVE and state.pressed and not state.eyes.empty()) {
        Eye &eye = state.eyes.back();
        eye.radius = cv::norm(eye.pos - to_vector(pos));
    }
    if (state.pressed or event == cv::EVENT_LBUTTONUP) {
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
    Face result{img, Rect{state.begin, state.end}, state.eyes[0], state.eyes[1]};
    return result;
}

void Face::render(const Bitmap3 &image) const {
    Bitmap3 result = image.clone();
    std::array<Vector2, 4> vertices{to_vector(region.tl()), Vector2(region.x, region.y + region.height), to_vector(region.br()), Vector2(region.x + region.width, region.y)};
    std::for_each(vertices.begin(), vertices.end(), [this](Vector2 &v) { v = tsf(v); });
    for (int i = 0; i < 4; i++) {
        cv::line(result, to_pixel(vertices[i]), to_pixel(vertices[(i+1)%4]), cv::Scalar(0, 1.0, 1.0));
    }
    for (const Eye &eye : eyes) {
        cv::circle(result, to_pixel(tsf(eye.pos)), eye.radius, cv::Scalar(0.5, 1.0, 0));
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
    const char winname[] = "calibrate";
    const Vector3 bgcolor(0.7, 0.6, 0.5);
    std::vector<std::pair<Vector2, Vector2>> measurements;
    Bitmap3 canvas(window_size.y, window_size.x, bgcolor);
    cv::namedWindow(winname);
    cv::imshow(winname, canvas);
    Vector2 cell(window_size.x / divisions, window_size.y / divisions);
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_real_distribution<> urand(0, 1);
    std::pair<Bitmap3, Vector2> task;
    while (1) {
        std::vector<Vector2> points;
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
            const int necessary_support = divisions * divisions;
            if (measurements.size() >= necessary_support) {
                int support;
                Gaze result(measurements, support, 100);
                if (support >= necessary_support) {
                    cv::destroyWindow(winname);
                    for (auto pair : measurements) {
                        std::cout << pair.first << " -> " << result(pair.first) << " vs. " << pair.second << std::endl;
                    }
                    return result;
                }
            }
            cv::waitKey(400);
            task.first.read(cap);
            task.second = point;
        }
    }
}
