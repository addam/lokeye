#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <vector>
#include <iostream>

const char *winname = "mirror";

struct Stopwatch
{
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point previous;
    Stopwatch() {
        previous = Clock::now();
    }
    float operator() () {
        auto now = Clock::now();
        float result = std::chrono::duration_cast<std::chrono::duration<float>>(now - previous).count();
        previous = now;
        return result;
    }
};

void show(float brightness)
{
    static cv::Mat canvas(1050, 1680, CV_32FC1);
    canvas = brightness;
    cv::imshow(winname, canvas);
    cv::waitKey(1);
}

void stripe(bool is_vertical, int pos, int grid=4)
{
    static cv::Mat canvas(1050, 1680, CV_32FC1);
    canvas = 0;
    int begin = pos * grid, end = (pos+1) * grid;
    is_vertical ? canvas.rowRange(begin, end) : canvas.colRange(begin, end) = 1;
    cv::imshow(winname, canvas);
    cv::waitKey(1);
}

cv::Mat capture(cv::VideoCapture &cam)
{
    auto start = std::chrono::steady_clock::now();
    float fps = cam.get(cv::CAP_PROP_FPS);
    for (Stopwatch time; time() * fps < 0.5; assert(cam.grab()));
    static cv::Mat img;
    cam.grab();
    cam.retrieve(img);
    cv::Mat result;
    img.convertTo(result, CV_32FC3, 1./255);
    cv::cvtColor(result, result, cv::COLOR_BGR2GRAY);
    return result;
}

int main()
{
    constexpr int trials = 10;
    cv::VideoCapture cam(0);
    cv::Mat black = capture(cam);
    black = 0;
    cv::Mat white = black.clone();
    for (int i=0; i<trials; ++i) {
        show(0);
        black += capture(cam);
        show(1);
        white += capture(cam);
    }
    cv::destroyWindow(winname);
    cv::Mat mask = (white - black) > 0.2 * trials;
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, cv::Mat());
    cv::imshow("mask", mask);
    cv::waitKey();
}
