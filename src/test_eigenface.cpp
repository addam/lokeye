#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include <iostream>

int main(int argc, char** argv)
{
    VideoCapture cam{0};
    Bitmap3 image;
    assert (image.read(cam));
    Face state = mark_eyes(image);
    std::cout << state.region << std::endl;
    Eigenface appearance(state);
    appearance.add(image, state.tsf);
    bool do_add = true;
    while (char(cv::waitKey(5)) != 27 and image.read(cam)) {
        state.refit(image);
        TimePoint time_start = std::chrono::high_resolution_clock::now();
        if (do_add) {
            appearance.add(image, state.tsf);
            std::cout << "adding image took ";
        } else {
            auto projection = appearance.evaluate(image, state.tsf);
            std::cout << projection << "; processing took ";
        }
        float duration = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - time_start).count();
        std::cout << duration << " s" << std::endl;
        state.render(image);
        char c = cv::waitKey(1);
        if (c == 27) {
            break;
        } else if (c == 'a') {
            do_add ^= true;
        }
    }
    return 0;
}
