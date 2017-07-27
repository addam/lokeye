#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
using namespace cv;
using namespace std;
const int q = 10;
int h_l, h_h=10, s_l, s_h=10, v_l, v_h=10;
Mat image;
const char* winname = "segmentation";

cv::Mat segmentation(const cv::Mat &img)
{
    int count = 0;
    using Vector3 = cv::Vec3f;
    cv::Mat tmp, result = img.clone();
    cvtColor(img, tmp, COLOR_BGR2HSV);
    for (int i=0; i<tmp.rows; ++i) {
        for (int j=0; j<tmp.cols; ++j) {
            Vector3 hsv = tmp.at<Vector3>(i, j);
            float h = q*hsv[0] / 360, s = q*hsv[1], v = q*hsv[2];
            if (h > h_l and h < h_h and s > s_l and s < s_h and v > v_l and v < v_h) {
                result.at<Vector3>(i, j) = Vector3(0, 0, 0);
                count += 1;
            }
        }
    }
    printf("%i hits\n", count);
    return result;
}

static void onTrackbar(int, void*)
{
    printf("h: %i..%i, s: %i..%i, v: %i..%i\n", h_l, h_h, s_l, s_h, v_l, v_h);
    auto canvas = segmentation(image);
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
    createTrackbar("h_l", winname, &h_l, q, onTrackbar);
    createTrackbar("h_h", winname, &h_h, q, onTrackbar);
    createTrackbar("s_l", winname, &s_l, q, onTrackbar);
    createTrackbar("s_h", winname, &s_h, q, onTrackbar);
    createTrackbar("v_l", winname, &v_l, q, onTrackbar);
    createTrackbar("v_h", winname, &v_h, q, onTrackbar);
    onTrackbar(0, 0);
    waitKey(0);
    return 0;
}

