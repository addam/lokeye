#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <cmath>
using namespace cv;
using namespace std;

int main(int argc, char** argv)
{
    Mat img, gray;
    if (argc < 2 or not (img=imread(argv[1], 1)).data) {
	    VideoCapture cam{0};
		cam.read(img);
	}
    cvtColor(img, gray, COLOR_BGR2GRAY);
    // smooth it, otherwise a lot of false circles may be detected
    //GaussianBlur(gray, gray, Size(9, 9), 2, 2);
    vector<Vec3f> circles;
	if (argc == 4 && std::string(argv[2]) == "-q") {
		float radius = atof(argv[3]);
	    HoughCircles(gray, circles, HOUGH_GRADIENT, 2, gray.rows/10, 200, 10, radius - 4, radius + 4);
	    if (circles.size()) {
			printf("%.2f %.2f\n", circles[0][0], circles[0][1]);
			return 0;
		} else {
			return 1;
		}
	}
    HoughCircles(gray, circles, HOUGH_GRADIENT, 2, gray.rows/10, 200, 10);
    for (size_t i = 0; i < circles.size(); i++) {
         Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
         int radius = cvRound(circles[i][2]);
         // draw the circle center
         circle(img, center, 2, Scalar(0,255,0), -1, 8, 0);
         // draw the circle outline
         circle(img, center, radius, Scalar(0,0,255), 1, 8, 0);
    }
    namedWindow("circles", 1);
    imshow("circles", img);
    waitKey(0);
    return 0;
}
