#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include <iostream>
#ifdef WITH_CERES
#include <glog/logging.h>
#endif

int main(int argc, char** argv)
{
    #ifdef WITH_CERES
    google::InitGoogleLogging("face.refit");
    #endif
    VideoCapture cam{0};
    Bitmap3 image;
    for (int i=0; i<10; i++) {
        assert (image.read(cam));
    }
    Face state = init_interactive(image);
    if (0) {
        auto serial = new SerialEye;
        auto hough = new HoughEye;
        auto limbus = new CorrelationEye;
        serial->add(FindEyePtr(hough));
        serial->add(FindEyePtr(limbus));
        state.eye_locator.reset(serial);
    } else if (0) {
        auto radial = new RadialEye(false);
        state.eye_locator.reset(radial);
    } else {
        // eye diameter is 85 / 100 of the image size
        auto bitmap = new BitmapEye("../data/iris.png", 85 / 100.f);
        state.eye_locator.reset(bitmap);
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
