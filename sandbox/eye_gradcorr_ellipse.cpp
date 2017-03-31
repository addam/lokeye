#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <cstdio>
#include <queue>
#include <tuple>
using namespace cv;
using namespace std;
int radius = 20;
const float blurCoef = 0.2;
int thr = 30;
Mat image, grad;
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
	float max = 0;
	for (int i=0; i<img.rows; ++i) {
		for (int j=0; j<img.cols; ++j) {
			Vec2f g = result.at<Vec2f>(i, j);
			if (norm(g) > max) {
				max = norm(g);
			}
		}
	}
	for (int i=0; i<img.rows; ++i) {
		for (int j=0; j<img.cols; ++j) {
			Vec2f &g = result.at<Vec2f>(i, j);
			if (100 * norm(g) > thr * max) {
				//g /= norm(g);
			} else {
				g *= 0;
			}
		}
	}
	return result;
}

vector<float> get_hint(Mat gray)
{
	vector<float> result;
	result.reserve(gray.cols * gray.rows);
	for (int i=0; i<gray.rows; ++i) {
		for (int j=0; j<gray.cols; ++j) {
			result.push_back((result.empty() ? 0 : result.back()) + gray.at<float>(i, j));
		}
	}
	return result;
}

Point random_point(Mat gray, vector<float> hint)
{
	float h = randu<float>() * hint.back();
	int i = lower_bound(hint.begin(), hint.end(), h) - hint.begin();
	return Point(i % gray.cols, i / gray.cols);
}

float ellipse_score(RotatedRect ellipse, Mat grad, Mat &canvas)
{
	float result = 0;
	float c = cos(M_PI * ellipse.angle / 180), s = sin(M_PI * ellipse.angle / 180);
	float a = ellipse.size.width / 2, b = ellipse.size.height / 2;
	for (int i=0; i < a + 1; ++i) {
		for (int j=0; j < a + 1; ++j) {
			float x = (c * j + s * i) / a, y = (c * i - s * j) / b;
			Vec2f normal(c * b * x - s * a * y, s * b * x + c * a * y);
			float r = pow2(x) + pow2(y);
			if (r > 1 - 2/b and r < 1 + 2/b and abs(normal[0]) + abs(normal[1]) != 0) {
				normal /= norm(normal);
				Point cn(ellipse.center.x, ellipse.center.y);
				array<Point, 4> points{Point(j, i) + cn, Point(j, -i) + cn, Point(-j, i) + cn, Point(-j, -i) + cn};
				for (Point p : points) {
					if (p.y >= 0 and p.y < grad.rows and p.x >= 0 and p.x < grad.cols) {
						//canvas.at<Vec3b>(p) = Vec3b(0, 128, 255);
						Vec2f g = grad.at<Vec2f>(p);
						result -= normal.dot(g);
					}
				}
			}
		}
	}
	//printf("score %g, %g = %g / %g\n", ellipse.center.x, ellipse.center.y, result, a*b);
	return result / (a * b);
}

float get_ellipse(const Mat &grad, Point center, float min_radius, float max_radius, RotatedRect &result, Mat &canvas)
{
	vector<Vec2f> points;
	for (int i=max(0.f, center.y - max_radius); i < center.y + max_radius and i < grad.rows; ++i) {
		for (int j=max(0.f, center.x - max_radius); j < center.x + max_radius and j < grad.cols; ++j) {
			Vec2f g = grad.at<Vec2f>(i, j);
			Vec2f direction(j - center.x, i - center.y);
			float r = norm(direction);
			if (r >= min_radius and r <= max_radius and g.dot(direction) > 0) {
				points.emplace_back(Vec2f(j , i));
			}
		}
	}
	//canvas.at<Vec3b>(center) = Vec3b(0, 0, 255);
	if (points.size() >= 5) {
		result = fitEllipse(points);
		auto sz = result.size;
		if (min(sz.width, sz.height) < 2 * min_radius or max(sz.width, sz.height) > 2 * max_radius) {
			//printf("get_ellipse fail: %gx%g\n", sz.width, sz.height);
			return 0;
		}
		for (Vec2f p : points) {
			//canvas.at<Vec3b>(round(p[1]), round(p[0])) = Vec3b(0, 255, 255);
		}
		return ellipse_score(result, grad, canvas);
	} else {
		//printf("get_ellipse fail: %i\n", int(points.size()));
		for (Vec2f p : points) {
			//canvas.at<Vec3b>(round(p[1]), round(p[0])) = Vec3b(0, 255, 128);
		}
		return 0;
	}
}

static void onTrackbar(int, void*)
{
	if (not radius) {
		return;
	}
	int blurSize = max(blurCoef * radius, 1.f);
	Point center(radius + blurSize, radius + blurSize);
	Mat tmpl = Mat::ones(2 * center.y, 2 * center.x, CV_32FC1), result;
	circle(tmpl, center, radius, 0.0, -1);
	blur(tmpl, tmpl, Size(blurSize, blurSize));
	//imshow("template", tmpl);
	tmpl = gradient(tmpl);
	matchTemplate(grad, tmpl, result, TM_CCORR_NORMED);
	//imshow("ccor", result);
	vector<Point> circles = maxima(result, max(10, result.cols / 10), center);
	Mat canvas(image.rows, max(2*image.cols, 200), CV_8UC3);
	canvas = 0;
	//printf("img: %ix%i, grad: %ix%i, tmpl': %ix%i, tmpl: %ix%i, ccor: %ix%i, canvas: %ix%i\n", image.rows, image.cols, grad.rows, grad.cols, 2*center.y, 2*center.x, tmpl.rows, tmpl.cols, result.rows, result.cols, canvas.rows, canvas.cols);
	image.copyTo(canvas.colRange(0, image.cols));
	for (int i=0; i<result.rows; ++i) {
		for (int j=0; j < result.cols; ++j) {
			float g = result.at<float>(i, j);
			canvas.at<Vec3b>(i + center.y, j + center.x + image.cols) = Vec3b(128, 128 + 128 * g, 128 - 128 * g);
		}
	}
	float best_score = 0;
	RotatedRect best;
    for (size_t i = 0; i < circles.size(); i++) {
		RotatedRect e;
		float score = get_ellipse(grad, circles[i], radius - blurSize, radius + blurSize, e, canvas);
		if (score > best_score) {
			best_score = score;
			best = e;
		}
        circle(canvas, circles[i] + Point(image.cols, 0), radius, Scalar(128, 255, 255), 1, 8, 0);
    }
    printf("score %g: %g, %g, %gx%g, %g\n", best_score, best.center.x, best.center.y, best.size.width, best.size.height, best.angle);
    if (best_score > 0) {
		ellipse(canvas, best, Scalar(128, 255, 0));
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
    Mat gray;
    cvtColor(image, gray, COLOR_BGR2GRAY);
    gray.convertTo(gray, CV_32FC1, 1./255);
    grad = gradient(gray);
    namedWindow(winname, 1);
    createTrackbar("T", winname, &thr, 100, onTrackbar);
    createTrackbar("R", winname, &radius, min(image.rows, image.cols) / 3, onTrackbar);
    onTrackbar(0, 0);
    waitKey(0);
    return 0;
}
