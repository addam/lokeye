#define WITH_OPENMP
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/video.hpp"
#include <stdio.h>
#include <string>
#include <chrono>
#include <queue>
using namespace std;
using namespace cv;

struct ValPoint : public cv::Point
{
public:
    float value;
    bool operator < (const ValPoint &other) const { return value > other.value; }
    ValPoint(float value, cv::Point p) : value{value}, cv::Point{p} {}
};
using Queue = std::priority_queue<ValPoint>;

cv::Point glob_init{-1, -1};

inline float pow2(float x)
{
    return x * x;
}

bool is_within(const cv::Mat img, cv::Point p)
{
    return p.x >= 0 and p.x < img.cols and p.y >= 0 and p.y < img.rows;
}

std::vector<cv::Point> floodfill(const cv::Mat &flow, cv::Point init)
{
    const cv::Point neighbors[] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    Queue q;
    int dead_perimeter = 0;
    q.push(ValPoint{0, init});
    const auto ref = flow.at<cv::Vec2f>(init);
    float level = 0;
    float bestCoef = 0;
    cv::Mat visited = cv::Mat::zeros(flow.size(), CV_8UC1);
    visited.at<uchar>(init) = 1;
    std::vector<cv::Point> result, extension;
    while (not q.empty()) {
        ValPoint p = q.top();
        q.pop();
        if (p.value > level) {
            float area = result.size() + extension.size();
            float circumference = pow2(q.size() + dead_perimeter);
            if (area > 20 and area / circumference > bestCoef) {
                bestCoef = area / circumference;
                std::move(extension.begin(), extension.end(), std::back_inserter(result));
                extension.clear();
            }
            level = p.value;
        }
        for (cv::Point n : neighbors) {
            n += p;
            if (is_within(flow, n) and not visited.at<uchar>(n)) {
                auto val = flow.at<cv::Vec2f>(n);
                float difference = pow2(ref[0] - val[0]) + pow2(ref[1] - val[1]);
                visited.at<uchar>(n) = 1;
                q.emplace(ValPoint{difference, n});
            } else if (not is_within(flow, n)) {
                dead_perimeter += 4;
            }
        }
        extension.push_back(p);
    }
    printf("filled %i pixels\n", result.size());
    return result;
}

static void onMouse(int event, int x, int y, int, void*)
{
	if (event == cv::EVENT_LBUTTONDOWN) {
        printf("set global init to %i, %i\n", x, y);
        glob_init = cv::Point{x, y};
    }
}

int main(int argc, char** argv)
{
    VideoCapture cap;
    bool update_bg_model = true;
    if(argc < 2) {
        cap.open(0);
    } else {
        cap.open(std::string(argv[1]));
    }
    if(!cap.isOpened()) {
        printf("Can not open camera or video file\n");
        return -1;
    }
    Mat prev_frame, tmp_frame;
    Mat tmp;
    cap >> tmp;
    cvtColor(tmp, prev_frame, CV_BGR2GRAY);
    if(prev_frame.empty()) {
        printf("can not read data from the video source\n");
        return -1;
    }
    namedWindow("video", 1);
    namedWindow("optflow", 1);
    cv::setMouseCallback("optflow", onMouse, 0);
    Ptr<DualTVL1OpticalFlow> algo = createOptFlow_DualTVL1();
    //algo->setInnerIterations(1);
    algo->setLambda(0.1);
    algo->setScaleStep(0.5);
    algo->setScalesNumber(20);
    algo->setEpsilon(0.015);
    algo->setInnerIterations(3);
    algo->setWarpingsNumber(2);
    algo->setMedianFiltering(3);
    printf("epsilon %g\ngamma %g\ninnerIterations %i\nlambda %g\nmedianFiltering %i\nouterIterations %i\nscalesNumber %i\nscaleStep %g\ntau %g\ntheta %g\nuseInitialFlow %s\nwarpingsNumber %i\n", algo->getEpsilon(), algo->getGamma(), algo->getInnerIterations(), algo->getLambda(), algo->getMedianFiltering(), algo->getOuterIterations(), algo->getScalesNumber(), algo->getScaleStep(), algo->getTau(), algo->getTheta(), algo->getUseInitialFlow()?"true":"false", algo->getWarpingsNumber());
    Mat optflow;
    while (1) {
        Mat tmp;
        cap >> tmp;
        cvtColor(tmp, tmp_frame, CV_BGR2GRAY);
        if( tmp_frame.empty() ) {
            break;
        }
        std::chrono::high_resolution_clock::time_point t_start = std::chrono::high_resolution_clock::now();
        algo->calc(prev_frame, tmp_frame, optflow);
        float elapsedTime = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - t_start).count();
        printf("calculation took %g seconds.\n", elapsedTime);
        Mat vis(tmp.size(), CV_32FC3);
        mixChannels(optflow, vis, {0, 0, 1, 1, -1, 2});
        //normalize(vis, vis, 0.0, 1.0, NORM_MINMAX);
        vis = vis * 0.1 + 0.5;
        if (is_within(optflow, glob_init)) {
            for (cv::Point p : floodfill(optflow, glob_init)) {
                vis.at<cv::Vec3f>(p)[2] = 1.f;
            }
        }
        imshow("video", tmp_frame);
        imshow("optflow", vis);
        char keycode = cv::waitKey(30);
        if( keycode == 27 ) {
            break;
        }
        std::swap(prev_frame, tmp_frame);
        if (not algo->getUseInitialFlow()) {
            algo->setUseInitialFlow(true);
            printf("useInitialFlow %s\n", algo->getUseInitialFlow()?"true":"false");
        }
    }
    return 0;
}

