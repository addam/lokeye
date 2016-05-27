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
    for (int i=0; char(cv::waitKey(5)) != 27 and image.read(cam); i++) {
        state.refit(image);
        std::cout << state.tsf.params << std::endl;
        state.render(image);
    }
    return 0;
}
