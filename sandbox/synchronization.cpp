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

void capture(cv::VideoCapture &cam)
{
    static auto start = std::chrono::steady_clock::now();
    cv::Mat img;
    cam >> img;
    float time = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - start).count();
    float average = hue(cv::mean(img.rowRange(img.rows/4, 3*img.rows / 4).colRange(img.cols / 4, 3*img.cols / 4)) / 255);
    printf("%f\t%i\t%f\n", time, state, average);
}

int main()
{
    const int max_delay = 20;
    switch_color();
    cv::waitKey(0);
    cv::VideoCapture cam(0);
    for (int i=0; i < max_delay; ++i) {
        capture(cam);
    }
    for (int delay=0; delay < max_delay; delay += 1) {
        switch_color();
        for (int i=0; i <= delay; ++i) {
            capture(cam);
        }
        switch_color();
        for (int i=0; i <= delay; ++i) {
            capture(cam);
        }
        switch_color();
        for (int i=0; i <= delay; ++i) {
            capture(cam);
        }
    }
    for (int i=0; i < max_delay; ++i) {
        capture(cam);
    }
}
