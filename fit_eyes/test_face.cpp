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
    TimePoint time_start = std::chrono::high_resolution_clock::now();
    for (int i=0; char(cv::waitKey(5)) != 27 and image.read(cam); i++) {
        state.refit(image);
        float duration = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - time_start).count();
        std::cout << state.tsf.params << ", " << 1 / duration << " fps" << std::endl;
        time_start = std::chrono::high_resolution_clock::now();
        state.render(image);
    }
    return 0;
}
