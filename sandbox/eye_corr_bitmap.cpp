#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <cstdio>
#include <queue>
#include <tuple>
#include "subpixel.hpp"
using namespace cv;
using namespace std;
int radius = 20;
Mat image;
vector<Mat> channels;
const char* winname = "cross-correlation with a bitmap";

bool local_maximum(const Mat &img, int i, int j)
{
	return (i == 0 or img.at<float>(i, j) >= img.at<float>(i - 1, j)) and
		(i == img.rows - 1 or img.at<float>(i, j) >= img.at<float>(i + 1, j)) and
		(j == 0 or img.at<float>(i, j) > img.at<float>(i, j - 1)) and
		(j == img.cols - 1 or img.at<float>(i, j) > img.at<float>(i, j + 1));
}

vector<Point> maxima(Mat src, unsigned count)
{
	using Record = std::tuple<float, int, int>;
	priority_queue<Record> heap;
	for (int i=0; i<src.rows; ++i) {
		for (int j=0; j<src.cols; ++j) {
			if (local_maximum(src, i, j)) {
				Record r = {-src.at<float>(i, j), j, i};
			    if (heap.size() < count) {
					heap.push(r);
				} else if (get<0>(heap.top()) >  get<0>(r)) {
					heap.pop();
					heap.push(r);
				}
			}
		}
	}
	vector<Point> result;
	while (not heap.empty()) {
		const Record &r = heap.top();
		result.emplace_back(Point(get<1>(r), get<2>(r)));
		heap.pop();
	}
	return result;
}

Mat run(float radius)
{
    Mat result;
    vector<Mat> imgc;
    cv::split(image, imgc);
    Mat mask;
    cv::resize(channels[3], mask, cv::Size(2*radius + 1, 2*radius + 1));
	for (int i=0; i<3; ++i) {
        Mat score, temp;
        cv::resize(channels[i], temp, mask.size());
        matchTemplate(imgc[i], temp, score, TM_CCORR_NORMED, mask);
        if (not result.data) {
            result = score;
        } else {
            result += score;
        }
    }
#if 0
    double min, max;
    cv::minMaxIdx(result, &min, &max);
    cv::imshow("score", (result - min) / (max - min));
    cv::waitKey();
#endif
    return result;
}

static void onTrackbar(int, void*)
{
	Mat score = run(radius + 1);
	vector<Point> circles = maxima(score, 10);
	Mat canvas = image.clone();
    for (size_t i = 0; i < circles.size(); i++) {
        circle(canvas, circles[i] + Point(radius, radius), radius, Scalar(0.0, 255*float(i)/circles.size(), 255-255*float(i)/circles.size()), 1, 8, 0);
    }
    imshow(winname, canvas);
}

void quiet_run(float radius, float &x, float &y)
{
    Mat score = run(radius);
	find_maximum(score, x, y);
	x += radius;
	y += radius;
}

int main( int argc, const char** argv )
{
    string filename(argv[1]);
    image = imread(filename, 1);
    Mat temp = imread("../data/iris.png", cv::IMREAD_UNCHANGED);
    if (image.empty() or temp.empty()) {
        printf("Cannot read image file.\n");
        return 1;
    }
    image.convertTo(image, CV_32F, 1./255);
    temp.convertTo(temp, CV_32F, 1./255);
    cv::split(temp, channels);
    printf("template has %lu channels.\n", channels.size());
    if (argc == 4 && std::string(argv[2]) == "-q") {
		radius = atoi(argv[3]);
		float x, y;
		quiet_run(radius, x, y);
		printf("%.2f %.2f\n", x, y);
		return 0;
	}
    namedWindow(winname, 1);
    createTrackbar("T", winname, &radius, 100, onTrackbar);
    onTrackbar(0, 0);
    waitKey(0);
    return 0;
}
