#include "main.h"
#include <vector>
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
    State(const char *winname, const Image &img) : winname{winname}, canvas{img.data} {
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

Face mark_eyes(Image &img)
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

void Face::render(const Image &image) const {
    Bitmap3 result = image.data.clone();
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
