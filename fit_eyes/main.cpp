#include "main.h"

int main(int argc, char** argv)
{
    Image reference_image;
    VideoCapture cam = (argc > 1) ? VideoCapture{argv[1]} : VideoCapture{0};
    reference_image.read(cam);
    Face state = mark_eyes(reference_image);
    #if 0
    while (cap.good()) {
        Image image;
        image.read(cam);
        state.refit(image, reference_image);
        imshow(state.render(image));
    }
    #endif
    return 0;
}
