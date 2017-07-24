#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include <iostream>

int main(int argc, char** argv)
{
    VideoCapture cam{0};
    Bitmap3 image;
    for (int i=0; i<10; i++) {
        assert (image.read(cam));
    }
    Face state = init_interactive(image);
    if (0) {
        auto serial = new SerialEye;
        auto hough = new HoughEye;
        auto limbus = new LimbusEye;
        serial->add(FindEyePtr(hough));
        serial->add(FindEyePtr(limbus));
        state.eye_locator.reset(serial);
    } else if (0) {
        state.eye_locator.reset(new CorrelationEye);
    } else if (1) {
        state.eye_locator.reset(new RadialEye(false));
    } else {
        // eye diameter is 85 / 100 of the image size
        state.eye_locator.reset(new BitmapEye("../data/iris.png", 85 / 100.f));
    }
    
    std::cout << state.main_region << std::endl;
    TimePoint time_start = std::chrono::high_resolution_clock::now();
    TimePoint time_prev = time_start;
    int i;
    for (i=0; char(cv::waitKey(1)) != 27 and image.read(cam); ++i) {
        state.refit(image);
        TimePoint time_now = std::chrono::high_resolution_clock::now();
        float duration = std::chrono::duration_cast<std::chrono::duration<float>>(time_now - time_prev).count();
        std::cout << "Main transformation shift: " << state.main_tsf.params.first << ", linear: " << state.main_tsf.params.second << ", face parameters: " << state() << ", " << 1 / duration << " fps" << std::endl;
        //std::cout << "Main transformation: " << state.main_tsf.params << ", face parameters: " << state() << ", " << 1 / duration << " fps" << std::endl;
        time_prev = time_now;
        state.render(image, "tracking");
    }
    float duration = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - time_start).count();
    printf("%i frames in %g seconds ~= %g fps\n", i, duration, i / duration);
    return 0;
}
