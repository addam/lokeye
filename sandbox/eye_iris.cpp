#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <cstdio>
#include <vector>

using cv::Point;
typedef unsigned char uchar;
const char *winname = "image";
const int max_radius = 20;
cv::Mat img, grad;

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

template<typename T>
struct Histogram : public std::vector<T>
{
	T mean() const;
	int count(T value, float delta=10) const;
};

template<typename T>
T Histogram<T>::mean() const
{
	using Parent = std::vector<T>;
	T result;
	for (int i=0; i < Parent::size(); ++i) {
		result = result * (float(i) / (i + 1)) + Parent::at(i) * (1 / float(i + 1));
	}
	return result;
}

template<typename T>
int Histogram<T>::count(T value, float delta) const
{
	using Parent = std::vector<T>;
	return std::count_if(Parent::begin(), Parent::end(), [value, delta](T other) { return norm(other - value) < delta; });
}

template<typename T>
T angle_address(const std::vector<T> &seq, cv::Vec2f v)
{
	if (v[0] == 0 and v[1] == 0) {
		return T();
	}
	int index = seq.size() * atan2(v[1], v[0]) / (2 * M_PI);
	return seq.at((index + seq.size()) % seq.size());
}

float eval_eye(Point center, bool verbose=false)
{
	using cv::Vec3b;
	using cv::Vec2f;
	std::vector<Histogram<Vec3b>> circles(max_radius);
	std::vector<float> gradsum(max_radius);
	for (int i=std::max(0, center.y - max_radius); i < img.rows and i <= center.y + max_radius; ++i) {
		for (int j=std::max(0, center.x - max_radius); j < img.cols and j <= center.x + max_radius; ++j) {
			Point p(j, i);
			Vec2f diff(j - center.x, i - center.y);
			int r = cv::norm(diff);
			if (r < max_radius) {
				circles.at(r).push_back(img.at<Vec3b>(p));
				gradsum.at(r) += diff.dot(grad.at<Vec2f>(p)) / pow2(r + 1);
			}
		}
	}
	int radius = std::max_element(gradsum.begin(), gradsum.end()) - gradsum.begin();
	if (verbose)  printf("radius: %i\n", radius);
	std::vector<float> angle_score(2 * radius * M_PI);
	for (int i=0; i<angle_score.size(); ++i) {
		float alpha = 2 * M_PI * i / angle_score.size();
		Vec2f diff(cos(alpha), sin(alpha));
		Point p(center.x + radius * diff[0], center.y + radius * diff[1]);
		if (p.x >= 0 and p.y >= 0 and p.x < grad.cols and p.y < grad.rows) {
			angle_score.at(i) = std::max(0.f, diff.dot(grad.at<Vec2f>(p)));
		} else {
			angle_score.at(i) = 0;
		}
	}
	float result = 0;
	cv::Mat canvas = verbose ? img.clone() : img;
	for (int i=std::max(0, center.y - radius); i < img.rows and i <= center.y + radius; ++i) {
		for (int j=std::max(0, center.x - radius); j < img.cols and j <= center.x + radius; ++j) {
			Point p(j, i);
			Vec2f diff(j - center.x, i - center.y);
			int r = cv::norm(diff);
			if (r < radius) {
				float as = angle_address(angle_score, diff);
				float rs = std::max(0.0, 1 - cv::norm(circles.at(r).mean() - img.at<Vec3b>(p)) / 256);
				float score = as * rs;
				result += score;
				if (verbose)  canvas.at<Vec3b>(p) = Vec3b(0, 255, 0) * score + Vec3b(0, 0, 255) * (1 - score);
			}
		}
	}
    if (verbose)  cv::imshow(winname, canvas);
	return result / pow2(radius + 1);
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
	Mat score(img.rows, img.cols, CV_32FC1);
	float maximum = 0;
	for (int i=1; i<score.rows - 1; ++i) {
		for (int j=1; j<score.cols - 1; ++j) {
			score.at<float>(i, j) = eval_eye(Point(j, i));
			maximum = std::max(maximum, score.at<float>(i, j));
		}
	}
	score /= maximum;
    imshow(winname, img);
    setMouseCallback(winname, onMouse);
    imshow("score", score);
    waitKey();
    return EXIT_SUCCESS;
}

