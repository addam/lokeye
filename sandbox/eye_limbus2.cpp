#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <cstdio>
#include <vector>

using cv::Point;
using cv::Vec2f;
using cv::Mat;
typedef unsigned char uchar;
const char *winname = "image";
using Curve = std::vector<Point>;

struct Circle
{
	Point center;
	float radius, score;
	bool operator< (const Circle &other) const {
		return score < other.score;
	}
	Circle operator* (float scale) const {
		return {scale * center, scale * radius, float(sqrt(scale)) * score};
	}
};

inline float pow2(float x)
{
    return x * x;
}

Mat gradient(const Mat &img)
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

const float grad_limit = 1.1;

float func(Vec2f grad, Vec2f direction)
{
	float n = cv::norm(grad);
	float d = grad.dot(direction) / n;
	return (n > grad_limit) ? d * (d + 1) : 0;//(d > 0) ? pow2(pow2(d)) * n : 0;
}

float eval_eye(const Mat &grad, Point center, const Curve &circle)
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

cv::Vec3f visualize(cv::Vec2f g) {
	float n = cv::norm(g);
	if (n <= grad_limit) {
		return cv::Vec3f(0, 0, 0);
	}
	g /= 2 * cv::norm(g);
	return cv::Vec3f(0.5, 0.5 + g[1], 0.5 + g[0]);
}

Circle find_eye(const Mat &img, float radius, bool verbose=false)
{
	if (radius > 40 and img.cols > 10) {
		Mat subimg;
		cv::pyrDown(img, subimg);
		float coef = img.cols / subimg.cols;
		return find_eye(subimg, radius / coef, verbose) * coef;
	}
	float maximum = 0;
	Point center;
	Mat grad = gradient(img);
	Curve circle = make_circle(radius);
	for (int i=0; i<grad.rows; ++i) {
		for (int j=0; j<grad.cols; ++j) {
			float val = eval_eye(grad, Point(j, i), circle);
			if (val > maximum) {
				maximum = val;
				center = Point(j, i);
			}
		}
	}
	if (verbose and radius > 1) {
		using cv::Vec3f;
		using cv::Vec2f;
		Mat canvas(img.rows, 3 * img.cols, CV_32FC3);
		img.convertTo(canvas.colRange(0, img.cols), CV_32F, 1./255);
		for (int i=0; i<img.rows; ++i) {
			for (int j=0; j<img.cols; ++j) {
				float val = eval_eye(grad, Point(j, i), circle) / maximum;
				canvas.at<Vec3f>(i, j + img.cols) = cv::Vec3f(val, val, val);
				canvas.at<Vec3f>(i, j + 2*img.cols) = visualize(grad.at<Vec2f>(i, j));
			}
		}
		for (Point d : make_circle(radius + 1)) {
			Point p = center + d;
			if (p.x >= 0 and p.y >= 0 and p.x < grad.cols and p.y < grad.rows) {
				canvas.at<Vec3f>(p.y, p.x + 2*img.cols) = visualize(Vec2f(p.x, p.y));
			}
		}
		cv::circle(canvas, center, radius, Vec3f(0, 0, 1), 1);
		cv::circle(canvas, center + Point(img.cols, 0), radius, Vec3f(0, 0, 1), 1);
		imshow(winname, canvas);
	}
	return Circle{center, radius, maximum};
}

int main(int argc, char* argv[])
{
	using namespace cv;
	Mat img;
	if (argc > 1) {
		img = imread(argv[1]);
	} else {
	    VideoCapture cam = VideoCapture(0);
		cam.read(img);
	}
	int max_radius = std::min(img.rows, img.cols) / 2;
	Circle circle;
	for (int r=1; r < max_radius; ++r) {
		Circle c = find_eye(img, r);
		if (circle < c) {
			circle = c;
		}
	}
	find_eye(img, circle.radius, true);
    waitKey();
    return EXIT_SUCCESS;
}

