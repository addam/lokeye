#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <cstdio>
#include <vector>

using cv::Point;
using cv::Vec2f;
typedef unsigned char uchar;
const char *winname = "image";
const int max_radius = 50;
cv::Mat img, grad;
using Curve = std::vector<Point>;

inline float pow2(float x)
{
    return x * x;
}

cv::Mat gradient(const cv::Mat &img)
{
	using namespace cv;
	Mat gray, d[2], result;
    cvtColor(img, gray, COLOR_BGR2GRAY);
    gray.convertTo(gray, CV_32FC1, 1./255);
	Sobel(gray, d[0], -1, 1, 0, -1);
	Sobel(gray, d[1], -1, 0, 1, -1);
	merge(d, 2, result);
	return result;
}

Curve make_circle(int r)
{
	Curve result;
	for (int x=0; ; ++x) {
		int y = sqrt(pow2(r) - pow2(x));
		if (y <= x) {
			break;
		} else {
			result.push_back(Point(x, y));
		}
	}
	result.reserve(8 * result.size() - 4);
	std::transform(result.begin(), result.end() - (result.back().x == result.back().y), back_inserter(result), [](Point p) { return Point(p.y, p.x); });
	std::transform(result.begin(), result.end() - 1, back_inserter(result), [](Point p) { return Point(p.x, -p.y); });
	std::transform(result.begin() + 1, result.end() - 1, back_inserter(result), [](Point p) { return Point(-p.x, p.y); });
	std::sort(result.begin(), result.end(), [](Point a, Point b) { return a.y < b.y or (a.y == b.y and a.x < b.x); });
	return result;
}

float func(Vec2f grad, Vec2f direction)
{
	float n = cv::norm(grad);
	float d = grad.dot(direction) / n;
	return (d > 0) ? pow2(pow2(d)) * n : 0;
}

float eval_eye(Point center, const Curve &circle, bool verbose=false)
{
	float weight = 0, result = 0;
	for (Point d : circle) {
		Point p = center + d;
		if (p.x >= 0 and p.y >= 0 and p.x < grad.cols and p.y < grad.rows) {
			Vec2f direction(d.x, d.y);
			direction /= cv::norm(direction);
			Vec2f g = grad.at<Vec2f>(p);
			result += func(g, direction);
			weight += 1;
		}
	}
	return (weight > 0) ? result / sqrt(weight) : 0;
}

static void onMouse(int event, int x, int y, int, void* param)
{
	static bool pressed = false;
	if (event == cv::EVENT_LBUTTONDOWN or (event == cv::EVENT_MOUSEMOVE and pressed)) {
		pressed = true;
		float result = eval_eye(Point(x, y), make_circle(10), true);
		printf("score: %g\n", result);
        //cv::imshow(winname, canvas);
    } else if (event == cv::EVENT_LBUTTONUP) {
		pressed = false;
	}
}

void quiet_run(float radius, float &x, float &y)
{
	Curve circle = make_circle(radius);
	float maximum = 0;
	for (int i=0; i<img.rows; ++i) {
		for (int j=0; j<img.cols; ++j) {
			float val = eval_eye(Point(j, i), circle);
			if (val > maximum) {
				maximum = val;
				x = j;
				y = i;
			}
		}
	}
}

int main(int argc, char* argv[])
{
	using namespace cv;
	if (argc > 1) {
		img = imread(argv[1]);
	} else {
	    VideoCapture cam = VideoCapture(0);
		cam.read(img);
	}
	grad = gradient(img);
	if (argc == 4 && std::string(argv[2]) == "-q") {
		float radius = atof(argv[3]);
		float x, y;
		quiet_run(radius, x, y);
		printf("%.2f %.2f\n", x, y);
		return 0;
	}
	Mat canvas(img.rows, 2 * img.cols, CV_32FC3);
	img.convertTo(canvas.colRange(0, img.cols), CV_32F, 1./255);
	float maximum = 0;
	Point center;
	int radius = 0;
	for (int r=1; r<max_radius; ++r) {
		Curve circle = make_circle(r);
		for (int i=0; i<img.rows; ++i) {
			for (int j=0; j<img.cols; ++j) {
				float val = eval_eye(Point(j, i), circle);
				if (val > maximum) {
					maximum = val;
					center = Point(j, i);
					radius = r;
				}
			}
		}
	}
	Curve circle = make_circle(radius);
	for (int i=0; i<img.rows; ++i) {
		for (int j=0; j<img.cols; ++j) {
			float val = eval_eye(Point(j, i), circle) / maximum;
			canvas.at<Vec3f>(i, j + img.cols) = Vec3f(val, val, val);
		}
	}
	cv::circle(canvas, center, radius, Vec3f(0, 0, 1), 1);
    imshow(winname, canvas);
    //setMouseCallback(winname, onMouse);
    //imshow("score", score);
    waitKey();
    return EXIT_SUCCESS;
}

