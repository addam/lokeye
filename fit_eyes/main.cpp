#include <iostream>
#include "main.h"

bool inside(Vector2 pos, Pixel size)
{
    return pos(0) >= 0 and pos(1) >= 0 and pos(0) < size.x and pos(1) < size.y;
}

int main(int argc, char** argv)
{
    Image reference_image;
    VideoCapture cam = (argc > 1) ? VideoCapture{argv[1]} : VideoCapture{0};
    for (int i=0; i<10; i++) {
        reference_image.read(cam);
    }
    Face state = mark_eyes(reference_image);
    Pixel size(1400, 700);
    Gaze fit = calibrate(state, cam, size);
    Bitmap3 record(size.y, size.x);
    record = Vector3(0.4, 0.3, 0.3);
    Image image;
    Vector2 prev_pos(-1, -1);
    for (int i=0; char(cv::waitKey(5)) != 27 and image.read(cam); i++) {
        //cv::waitKey();
        state.refit(image);
        Vector2 pos = fit(state());
        std::cout << "position: " << pos(0) << ", " << pos(1) << std::endl;
        state.render(image);
        if (inside(pos, size) and inside(prev_pos, size)) {
            cv::line(record, to_pixel(prev_pos), to_pixel(pos), cv::Scalar(0.5, 0.7, 1));
        }
        if (i % 5 == 0) {
            cv::imshow("record", record);
        }
        prev_pos = pos;
    }
    return 0;
}
