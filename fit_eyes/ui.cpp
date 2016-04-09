#include "main.h"
#include <vector>

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
    State(const char *winname, const Image &img) : winname{winname}, canvas{img.data} {
        cv::namedWindow(winname);
        cv::setMouseCallback(winname, onMouse, this);
        cv::imshow(winname, canvas);
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

Face mark_eyes(Image &img)
{
    State state("mark eyes", img);
    do {
        cv::waitKey(100);
    } while (not state.done());
    Face result{img, Rect{state.begin, state.end}, state.eyes[0], state.eyes[1]};
    return result;
}

void Face::render(const Image &image) const {
    const char winname[] = "fit";
    Bitmap3 result = image.data.clone();
    cv::RotatedRect shape{tsf(Vector2(region.x + 0.5 * region.width, region.y + 0.5 * region.height)), cv::Size2f(region.width, region.height), 180 * tsf.params[2] / float(M_PI)};
    cv::Point2f vertices[4];
    shape.points(vertices);
    for (int i = 0; i < 4; i++) {
        cv::line(result, vertices[i], vertices[(i+1)%4], cv::Scalar(0, 1.0, 1.0));
    }
    for (const Eye &eye : eyes) {
        cv::circle(result, to_pixel(tsf(eye.pos)), eye.radius, cv::Scalar(0.5, 1.0, 0));
    }
    cv::imshow(winname, result);
}