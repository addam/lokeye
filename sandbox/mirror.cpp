// mirror: předpokládá, že kus obrazovky je vidět odražený v kameře.
// ukazuje černobílé vzory a hledá jejich lineární zobrazení na vstup z kamery
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <vector>
#include <tuple>
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

class Mirror
{
    const float threshold = 0.1;
    const int grid = 8;
    const int width = 1680, height = 1050;
    cv::VideoCapture &cam;
    cv::Mat mask;
    
    void stripe(bool is_vertical, int pos, int thickness=1) {
        static cv::Mat canvas(height, width, CV_32FC1);
        canvas = 0;
        int begin = std::max(0, pos * grid), end = std::min((pos + thickness)*grid, (is_vertical ? width : height));
        if (pos >= 0 and end > begin) {
            (is_vertical ? canvas.colRange(begin, end) : canvas.rowRange(begin, end)) = 1;
        }
        cv::imshow(winname, canvas);
        cv::waitKey(1);
    }
    
    float importance(cv::Mat &&img) const {
        cv::multiply(img, mask, img, 1, CV_32FC1);
        return cv::sum(cv::abs(img))[0];
    }
    
    void block(int x, int y) {
        static cv::Mat canvas(height, width, CV_32FC1);
        canvas = 0;
        canvas.rowRange(y*grid, (y+1)*grid).colRange(x*grid, (x+1)*grid) = 1;
        cv::imshow(winname, canvas);
        cv::waitKey(1);
    }
    
    void init_mask(int trials=5)
    {
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
        mask = (white - black) > threshold * trials;
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, cv::Mat());
        std::vector<std::vector<cv::Point>> contours;
        findContours(mask, contours, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);
        if (contours.empty()) {
            printf("mask area 0\n");
            throw 1;
        }
        int largestComp = 0, maxArea = 0;
        for(int i=0; i < contours.size(); ++i) {
            double area = std::abs(cv::contourArea(contours[i]));
            if (area > maxArea) {
                maxArea = area;
                largestComp = i;
            }
        }
        printf("mask area %i\n", maxArea);
        mask = 0;
        cv::drawContours(mask, contours, largestComp, cv::Scalar(255), cv::FILLED, cv::LINE_8);
        cv::waitKey();
    }
    
    std::vector<int> sliding_stripe(bool is_vertical, int thickness, int limit=2)
    {
        int bound = (is_vertical?width:height) / grid;
        struct WeightedInt {
            int value;
            float weight;
            bool operator<(const WeightedInt &other) const {
                return weight > other.weight;
            }
        };
        std::vector<WeightedInt> order;
        stripe(is_vertical, -1);
        cv::Mat black = capture(cam);
        for (int i=0; i < bound; i+=thickness) {
            stripe(is_vertical, i, thickness);
            cv::Mat here = capture(cam);
            order.push_back({i, importance(here - black)});
            stripe(is_vertical, -1);
            black = capture(cam);
        }
        std::sort(order.begin(), order.end());
        for (int i=0; i<limit and i < order.size(); ++i) {
            printf("choose %i with weight %g\n", order[i].value, order[i].weight);
        }
        std::vector<int> result;
        std::transform(order.begin(), std::min(order.end(), order.begin() + limit), std::back_inserter(result), [](WeightedInt i){ return i.value; });
        return result;
    }
    
public:
    Mirror(cv::VideoCapture &cam) : cam(cam) {
        int speedup = 8;
        std::vector<int> rows, columns;
        init_mask();
        columns = sliding_stripe(true, speedup);
        rows = sliding_stripe(false, speedup);
        cv::Mat canvas(height, width, CV_32FC1);
        canvas = 0;
        for (int x : columns) {
            for (int y : rows) {
                canvas.colRange(x*grid, std::min((x+speedup)*grid, width)).rowRange(y*grid, std::min((y+speedup)*grid, height)) = 1;
            }
        }
        cv::imshow("important", canvas);
        cv::waitKey();
        //for (i < n) {
            //show and record random()
        //}
        //invert
        //choose 16 top
    }
    
    void watermark(cv::Mat &img, int value) {
        
    }
    
    int operator()(const cv::Mat &img) {
        
    }
};

int main()
{
    cv::VideoCapture cam(0);
    for (int i=0; i<10; ++i) {
        capture(cam);
    }
    Mirror mirror(cam);
}
