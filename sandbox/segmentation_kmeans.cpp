#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
using namespace cv;
using namespace std;
int trackbar = 1;
Mat image;
const char* winname = "segmentation";

#if 0
Mat segmentation(const Mat_<Vec3f> &img, int count=3)
{
    means = init(img, count);
    for (int i=0; i<100; ++i) {
        means = reassign(img);
    }
    Mat result(img.size(), CV_8UC1);
    for (int i=0; i<result.rows; ++i) {
        for (int j=0; j<result.cols; ++j) {
            Point p(j, i);
            result.at<uchar>(p) = nearest(means, img.at<Vec3f>(p));
        }
    }
}
#else
cv::Mat segmentation(const cv::Mat &img, int count=3)
{
    cv::Mat result;
    cv::Mat samples;
    cvtColor(img, samples, COLOR_BGR2Lab);
    samples = samples.reshape(1, samples.size().area());
    cv::kmeans(samples, count, result, TermCriteria(TermCriteria::EPS+TermCriteria::COUNT, 10, 1.0), 3, KMEANS_PP_CENTERS);
    return result.reshape(1, img.rows);
}
#endif

static void onTrackbar(int, void*)
{
	int count = trackbar + 2;
	Mat canvas = image.clone();
    vector<vector<Point>> contours;
    findContours(segmentation(image), contours, RETR_CCOMP, CHAIN_APPROX_SIMPLE);
    drawContours(canvas, contours, -1, Vec3f(0, 0, 1), 1, 4);
    imshow(winname, canvas);
}

int main( int argc, const char** argv )
{
    string filename(argv[1]);
    imread(filename, 1).convertTo(image, CV_32FC3, 1./255);
    if (image.empty()) {
        printf("Cannot read image file: %s\n", filename.c_str());
        return 1;
    }
    namedWindow(winname, 1);
    createTrackbar("K", winname, &trackbar, 10, onTrackbar);
    onTrackbar(0, 0);
    waitKey(0);
    return 0;
}

