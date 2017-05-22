#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <cstdio>
#include <vector>

using cv::Point;
typedef unsigned char uchar;
const char *winname = "image";
const int max_radius = 50;
cv::Mat img, grad;

const float radius_exponent = 1.5;
const float grad_limit = 1.1;

inline float pow2(float x)
{
    return x * x;
}

float grad_func(cv::Vec2f grad, cv::Vec2f direction)
{
	if (direction[0] == 0 and direction[1] == 0) {
		return 0;
	}
	float n = cv::norm(grad);
	float d = grad.dot(direction) / (n * cv::norm(direction));
	return (d > 0) ? pow2(pow2(d)) * n : 0;
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

template<typename T>
struct Histogram
{
	T mean;
	//int size;
	std::vector<T> data;
	void insert(const T&);
};

template<typename T>
void Histogram<T>::insert(const T &value)
{
	data.push_back(value);
	auto mid = data.begin() + data.size() / 2;
	std::nth_element(data.begin(), mid, data.end(), [](T l, T r) { return cv::norm(l) < cv::norm(r); });
	mean = *mid;
	//mean = mean * (size / float(size + 1)) + value / float(size + 1);
	//size += 1;
}

template<typename T>
T &angle_address(std::vector<T> &seq, cv::Vec2f v)
{
	assert (v[0] != 0 or v[1] != 0);
	int index = seq.size() * atan2(v[1], v[0]) / (2 * M_PI);
	return seq.at((index + seq.size()) % seq.size());
}

float eye_radius(Point center)
{
	using cv::Vec3b;
	using cv::Vec2f;
	std::vector<float> gradsum(max_radius);
	for (int i=std::max(0, center.y - max_radius); i < img.rows and i <= center.y + max_radius; ++i) {
		for (int j=std::max(0, center.x - max_radius); j < img.cols and j <= center.x + max_radius; ++j) {
			Point p(j, i);
			Vec2f diff(j - center.x, i - center.y);
			int r = cv::norm(diff);
			if (r < max_radius) {
				gradsum[r] += grad_func(grad.at<Vec2f>(p), diff);
			}
		}
	}
	for (int i=0; i<max_radius; ++i) {
		gradsum[i] /= pow(i + 1, radius_exponent);
	}
	return std::max_element(gradsum.begin(), gradsum.end()) - gradsum.begin();
}

float eval_eye(Point center, bool verbose=false, int radius=0)
{
	using cv::Vec3b;
	using cv::Vec2f;
	if (radius == 0) {
		radius = eye_radius(center);
	}
	if (verbose)  printf("radius: %i\n", radius);
	// calculate average over each radius
	std::vector<Histogram<Vec3b>> circles(radius);
	for (int i=std::max(0, center.y - radius); i < img.rows and i <= center.y + radius; ++i) {
		for (int j=std::max(0, center.x - radius); j < img.cols and j <= center.x + radius; ++j) {
			Point p(j, i);
			Vec2f diff(j - center.x, i - center.y);
			int r = cv::norm(diff);
			if (r < radius) {
				circles[r].insert(img.at<Vec3b>(p));
			}
		}
	}
	// calculate score of each angle, based on the gradient at its end
	std::vector<float> angle_score(2 * radius * M_PI);
	for (int i=0; i<angle_score.size(); ++i) {
		float alpha = 2 * M_PI * i / angle_score.size();
		Vec2f diff(cos(alpha), sin(alpha));
		Point p(center.x + radius * diff[0], center.y + radius * diff[1]);
		if (p.x >= 0 and p.y >= 0 and p.x < grad.cols and p.y < grad.rows) {
			angle_score.at(i) = grad_func(grad.at<Vec2f>(p), diff);
		} else {
			angle_score.at(i) = 0;
		}
	}
	cv::Mat canvas;
	if (verbose) {
		canvas = cv::Mat(img.rows, 2 * img.cols, CV_8UC3);
		img.copyTo(canvas.colRange(0, img.cols));
		img.copyTo(canvas.colRange(img.cols, 2*img.cols));
	}
	std::vector<float> angle_score2(2 * radius * M_PI, 0);
	std::vector<float> angle_weight(2 * radius * M_PI, 0);
	for (int i=std::max(0, center.y - radius); i < img.rows and i <= center.y + radius; ++i) {
		for (int j=std::max(0, center.x - radius); j < img.cols and j <= center.x + radius; ++j) {
			Point p(j, i);
			Vec2f diff(j - center.x, i - center.y);
			int r = cv::norm(diff);
			if (0 < r and r < radius) {
				float rs = std::max(0.0, 1 - cv::norm(circles.at(r).mean - img.at<Vec3b>(p)) / 20);
				angle_address(angle_score2, diff) += rs;
				angle_address(angle_weight, diff) += 1;
				if (verbose)  canvas.at<Vec3b>(p) = Vec3b(0, 255, 0) * rs + Vec3b(0, 0, 255) * (1 - rs);
				if (verbose)  canvas.at<Vec3b>(p + Point(img.cols, 0)) = circles[r].mean;
			} else if (verbose and r == radius) {
				float as = angle_address(angle_score, diff);
				if (verbose)  canvas.at<Vec3b>(p + Point(img.cols, 0)) = Vec3b(0, 255, 0) * as + Vec3b(0, 0, 255) * (1 - as);
			}
		}
	}
	float result = 0;
	for (int i=0; i<angle_score.size(); ++i) {
		if (angle_weight[i] > 0) {
			result += angle_score[i] * pow2(angle_score2[i] / angle_weight[i]);
		}
	}
    if (verbose)  cv::imshow(winname, canvas);
	return result * pow(radius + 1, 2 - radius_exponent);
}

static void onMouse(int event, int x, int y, int, void* param)
{
	static bool pressed = false;
	if (event == cv::EVENT_LBUTTONDOWN or (event == cv::EVENT_MOUSEMOVE and pressed)) {
		pressed = true;
		float result = eval_eye(Point(x, y), true);
		printf("score: %g\n", result);
        //cv::imshow(winname, canvas);
    } else if (event == cv::EVENT_LBUTTONUP) {
		pressed = false;
	}
}

void quiet_run(float radius, float &x, float &y)
{
	float maximum = 0;
	for (int i=0; i<img.rows; ++i) {
		for (int j=0; j<img.cols; ++j) {
			float val = eval_eye(Point(j, i), false, radius);
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
		if (radius > 30) {
			// it would take ages to compute this
			return 1;
		}
		float x, y;
		quiet_run(radius, x, y);
		printf("%.2f %.2f\n", x, y);
		return 0;
	}
	Mat canvas(img.rows, 2 * img.cols, CV_32FC3);
	img.convertTo(canvas.colRange(0, img.cols), CV_32F, 1./255);
	float maximum = 0;
	Point center;
	for (int i=0; i<img.rows; ++i) {
		for (int j=0; j<img.cols; ++j) {
			float val = eval_eye(Point(j, i));
			canvas.at<Vec3f>(i, j + img.cols) = Vec3f(val, val, val);
			if (val > maximum) {
				maximum = val;
				center = Point(j, i);
			}
		}
	}
	printf("best score: %g\n", maximum);
	canvas.colRange(img.cols, 2 * img.cols) /= maximum;
	int radius = eye_radius(center);
	circle(canvas, center, radius, Vec3f(0, 0, 1), 1);
    imshow(winname, canvas);
    setMouseCallback(winname, onMouse);
    //imshow("score", score);
    waitKey();
    return EXIT_SUCCESS;
}

