#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <cstdio>
#include <queue>
#include <tuple>
using namespace cv;
using namespace std;
int radius = 20;
const float blurCoef = 0.2;
int thr = 0;
Mat image, gray;
const char* winname = "gradient cross-correlation with a circle";

float pow2(float x)
{
	return x * x;
}

bool local_maximum(const Mat &img, int x, int y)
{
	return (x == 0 or img.at<float>(y, x) >= img.at<float>(y, x - 1)) and
		(x == img.cols - 1 or img.at<float>(y, x) >= img.at<float>(y, x + 1)) and
		(y == 0 or img.at<float>(y, x) > img.at<float>(y - 1, x)) and
		(y == img.rows - 1 or img.at<float>(y, x) > img.at<float>(y + 1, x));
}

vector<Point> maxima(Mat src, unsigned count, Point offset={0, 0})
{
	using Record = std::tuple<float, int, int>;
	priority_queue<Record> heap;
	for (int i=0; i<src.rows; ++i) {
		for (int j=0; j<src.cols; ++j) {
			if (local_maximum(src, j, i)) {
				Record r = {-src.at<float>(i, j), j + offset.x, i + offset.y};
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

Mat gradient(Mat img)
{
	Mat d[2], result;
	Sobel(img, d[0], -1, 1, 0, -1);
	Sobel(img, d[1], -1, 1, 0, -1);
	merge(d, 2, result);
	return result;
}

Mat polynomial(Mat img)
{
	Mat p[5], result;
	split(img, p);
	multiply(p[0], p[0], p[2]);
	multiply(p[0], p[1], p[3], 2);
	multiply(p[1], p[1], p[4]);
	merge(p, 5, result);
	return result;
}

void normalize(Mat &img, bool use_pow2=false)
{
	const int channels = img.channels();
	vector<float> average(channels);
	for (int i=0; i<img.rows; ++i) {
		const float *p = img.ptr<const float>(i);
		for (int j=0; j < img.cols; ++j) {
			for (int c=0; c<channels; ++c) {
				if (use_pow2) {
					average[c] += pow2(p[channels * j + c]);
				} else {
					average[c] += p[channels * j + c];
				}
			}
		}
	}
	for (float &f : average) {
		f = (img.rows * img.cols) / f;
	}
	for (int i=0; i<img.rows; ++i) {
		float *p = img.ptr<float>(i);
		for (int j=0; j < img.cols; ++j) {
			for (int c=0; c<channels; ++c) {
				p[channels * j + c] *= average[c];
			}
		}
	}
}

void normalize(Mat &img, const Mat &mask)
{
	vector<Mat> splitted;
	split(img, splitted);
	Mat tmp;
	for (Mat &m : splitted) {
		matchTemplate(m, mask, tmp, TM_CCORR);
		m -= tmp;
		pow(m, 2, tmp);
		matchTemplate(tmp, mask, tmp, TM_CCORR);
		divide(m, tmp, m);
	}
	merge(splitted, img);
}

void ncorr(const Mat &img, const Mat &tmpl, Mat &result)
{
	Mat grad_img = gradient(img), grad_tmpl = gradient(tmpl);
	Mat mask = abs(grad_tmpl);
	normalize(grad_img);
	matchTemplate(polynomial(grad_img), polynomial(grad_tmpl), result, TM_CCORR);
}

static void onTrackbar(int, void*)
{
	if (not radius) {
		return;
	}
    Mat grad = gradient(gray);
	int blurSize = blurCoef * radius;
	Point center(radius + blurSize, radius + blurSize);
	Mat tmpl = Mat::ones(2 * center.y, 2 * center.x, CV_32FC1), result;
	circle(tmpl, center, radius, 0.0, -1);
	if (blurSize > 0) {
		blur(tmpl, tmpl, Size(blurSize, blurSize));
	}
	tmpl = gradient(tmpl);
	ncorr(grad, tmpl, result);
	imshow("ccor", result);
	vector<Point> circles = maxima(result, max(10, result.cols / 10), center);
	Mat canvas(image.rows, max(2*image.cols, 200), CV_8UC3);
	canvas = 0;
	image.copyTo(canvas.colRange(0, image.cols));
	for (int i=0; i<result.rows; ++i) {
		for (int j=0; j < result.cols; ++j) {
			float g = result.at<float>(i, j);
			canvas.at<Vec3b>(i + center.y, j + center.x + image.cols) = Vec3b(128, 128 + 128 * g, 128 - 128 * g);
		}
	}
    for (size_t i = 0; i < circles.size(); i++) {
        circle(canvas, circles[i], radius, Scalar(128, 255, 255), 1, 8, 0);
        canvas.at<Vec3b>(circles[i]) = canvas.at<Vec3b>(circles[i] + Point(image.cols, 0)) = Vec3b(0, 0, 255);
    }
    imshow(winname, canvas);
}

int main(int argc, const char** argv)
{
	if (argc > 1) {
	    image = imread(argv[1], 1);
	    if(image.empty()) {
	        printf("Cannot read image file: %s\n", argv[1]);
	        return 1;
	    }
	} else {
	    VideoCapture cam(0);
		cam.read(image);
	}
    cvtColor(image, gray, COLOR_BGR2GRAY);
    gray.convertTo(gray, CV_32FC1, 1./255);
    namedWindow(winname, 1);
    createTrackbar("T", winname, &thr, 100, onTrackbar);
    createTrackbar("R", winname, &radius, min(image.rows, image.cols) / 3, onTrackbar);
    onTrackbar(0, 0);
    waitKey(0);
    return 0;
}
