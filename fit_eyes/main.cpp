#include "main.h"

int main(int argc, char** argv)
{
    Image reference_image;
    VideoCapture cam = (argc > 1) ? VideoCapture{argv[1]} : VideoCapture{0};
    reference_image.read(cam);
    Face state = mark_eyes(reference_image);
    Image image;
    while (char(cv::waitKey(20)) != 27 and image.read(cam)) {
        state.refit(image);
        state.render(image);
        printf("done\n");
    }
    return 0;
}
