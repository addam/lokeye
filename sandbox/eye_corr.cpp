#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <cstdio>
#include <queue>
#include <tuple>
using namespace cv;
using namespace std;
int radius = 20;
Mat image, gray;
const char* winname = "cross-correlation with a circle";

bool local_maximum(const Mat &img, int x, int y)
{
	return (x == 0 or img.at<float>(y, x) >= img.at<float>(x - 1, y)) and
		(x == img.cols - 1 or img.at<float>(y, x) >= img.at<float>(x + 1, y)) and
		(y == 0 or img.at<float>(y, x) > img.at<float>(x, y - 1)) and
		(y == img.rows - 1 or img.at<float>(y, x) > img.at<float>(x, y + 1));
}

vector<Point> maxima(Mat src, unsigned count)
{
	using Record = std::tuple<float, int, int>;
	priority_queue<Record> heap;
	for (int i=0; i<src.rows; ++i) {
		for (int j=0; j<src.cols; ++j) {
			if (local_maximum(src, j, i)) {
				Record r = {-src.at<float>(j, i), j, i};
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

static void onTrackbar(int, void*)
{
	const int offset = 1.5 * radius;
	Mat tmpl = Mat::ones(2 * offset, 2 * offset, CV_32FC1), result;
	circle(tmpl, Point(offset, offset), radius, 0.0, -1);
	//imshow("template", tmpl);
	matchTemplate(gray, tmpl, result, TM_CCORR_NORMED);
	//imshow("ccor", result);
	vector<Point> circles = maxima(result, 10);
	Mat canvas = image.clone();
    for (size_t i = 0; i < circles.size(); i++) {
         circle(canvas, circles[i] + Point(offset, offset), radius, Scalar(0.0, 255*float(i)/circles.size(), 255-255*float(i)/circles.size()), 1, 8, 0);
    }
    imshow(winname, canvas);
}

void quiet_run(float radius, float &x, float &y)
{
	const int offset = 1.5 * radius;
	Mat tmpl = Mat::ones(2 * offset, 2 * offset, CV_32FC1), score;
	circle(tmpl, Point(offset, offset), radius, 0.0, -1);
	matchTemplate(gray, tmpl, score, TM_CCORR_NORMED);
	Point result = maxima(score, 1)[0];
	x = result.x + offset;
	y = result.y + offset;
}

int main( int argc, const char** argv )
{
    string filename(argv[1]);
    image = imread(filename, 1);
    if (image.empty()) {
        printf("Cannot read image file: %s\n", filename.c_str());
        return 1;
    }
    cvtColor(image, gray, COLOR_BGR2GRAY);
    gray.convertTo(gray, CV_32FC1, 1./255);
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
