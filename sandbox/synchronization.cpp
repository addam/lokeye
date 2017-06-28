// synchronization: předpokládá, že kamera přímo zabírá monitor
// mění cyklicky barvu monitoru a zaznamenává barvu na kameře, aby šlo odhadnout zpoždění
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <chrono>
#include <vector>
#include <iostream>

int state = -1;

float hue(cv::Scalar color)
{
    int m = (color[0] < std::min(color[1], color[2])) ? 0 : (color[1] < color[2]) ? 1 : 2;
    int l = (m + 1) % 3, r = (m + 2) % 3;
    return 2 - (l + (color[r] - color[m]) / (color[l] + color[r] - 2 * color[m]));
}

void switch_color()
{
    const char *winname = "synchronization";
    const std::vector<cv::Scalar> colors = {{0, 0, 1}, {0, 1, 0}, {1, 0, 0}};
    static cv::Mat canvas(1000, 1000, CV_32FC3);
    state = (state + 1) % colors.size();
    canvas = colors[state];
    cv::imshow(winname, canvas);
    cv::waitKey(1);
}

float capture(cv::VideoCapture &cam, float &time)
{
    auto start = std::chrono::steady_clock::now();
    while (1) {
        // make sure that we got a new image
        auto frame_start = std::chrono::steady_clock::now();
        if (not cam.grab()) {
            return false;
        }
        time = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - frame_start).count() * cam.get(cv::CAP_PROP_FPS);
        if (time > 0.5) {
            break;
        }
    }
    cv::Mat img;
    cam.grab();
    time = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - start).count() * cam.get(cv::CAP_PROP_FPS);
    cam.retrieve(img);
    return hue(cv::mean(img.rowRange(img.rows/4, 3*img.rows / 4).colRange(img.cols / 4, 3*img.cols / 4)) / 255);
}

int main()
{
    const int max_delay = 20;
    float time;
    switch_color();
    cv::waitKey(0);
    cv::VideoCapture cam(0);
    for (int i=0; i<30; ++i) {
        switch_color();
        for (int j=0; j < max_delay; ++j) {
            if (std::round(capture(cam, time)) == state) {
                printf("%i\t%f\n", j, time);
                break;
            }
            printf("%i\t%f...\n", j, time);
        }
    }
}
