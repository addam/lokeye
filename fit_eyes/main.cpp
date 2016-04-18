#include "main.h"

int main(int argc, char** argv)
{
    Image reference_image;
    VideoCapture cam = (argc > 1) ? VideoCapture{argv[1]} : VideoCapture{0};
    reference_image.read(cam);
    Face state = mark_eyes(reference_image);
    Image image;
    for (int i=0; char(cv::waitKey(5)) != 27 and image.read(cam); i++) {
        //cv::waitKey();
        state.refit(image, 0);
        state.render(image);
    }
    return 0;
}
