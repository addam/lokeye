#include "main.h"
#include <vector>

namespace {
void onMouse(int, int, int, int, void*);

struct State
{
    const char *winname;
    const Bitmap &canvas;
    Vec2f begin, end;
    std::vector<Eye> eyes;
    bool has_rectangle = false;
    bool pressed = false;
    State(const char *winname, const Image &img) : winname{winname}, canvas{img.data} {
        cv::namedWindow(winname);
        cv::setMouseCallback(winname, onMouse, this);
        cv::imshow(winname, canvas);
    }
    void render() const {
        Bitmap result = canvas.clone();
        if (has_rectangle) {
            cv::rectangle(result, round(begin), round(end), cv::Scalar(0, 1.0, 1.0));
        }
        for (const Eye &eye : eyes) {
            cv::circle(result, round(eye.pos), eye.radius, cv::Scalar(0.5, 1.0, 0));
        }
        cv::imshow(winname, result);
    }
    bool done() const {
        return has_rectangle and eyes.size() >= 2 and not pressed;
    }
};

void onMouse(int event, int x, int y, int, void* param)
{
    Vec2f pos(x, y);
    State &state = *static_cast<State*>(param);
    if (event == cv::EVENT_LBUTTONDOWN) {
        state.pressed = true;
    } else if (event == cv::EVENT_LBUTTONUP) {
        state.pressed = false;
    }
    if (event == cv::EVENT_LBUTTONDOWN and not state.has_rectangle) {
        state.begin = pos;
    } else if (event == cv::EVENT_LBUTTONDOWN and state.has_rectangle) {
        state.eyes.emplace_back(Eye{pos, 1.0f});
    } else if (event == cv::EVENT_MOUSEMOVE and state.pressed and state.eyes.empty()) {
        state.end = pos;
        state.has_rectangle = true;
    } else if (event == cv::EVENT_MOUSEMOVE and state.pressed and not state.eyes.empty()) {
        Eye &eye = state.eyes.back();
        eye.radius = cv::norm(eye.pos - pos);
    }
    if (state.pressed or event == cv::EVENT_LBUTTONUP) {
        state.render();
    }
}

}

Face mark_eyes(const Image &img)
{
    State state("mark eyes", img);
    do {
        cv::waitKey(100);
    } while (not state.done());
    Face result;
    result.region = {state.begin, state.end};
    result.eyes[0] = state.eyes[0];
    result.eyes[1] = state.eyes[1];
    return result;
}
