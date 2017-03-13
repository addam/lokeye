#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <cstdio>
using namespace cv;
using namespace std;
int edgeThresh = 1;
Mat image, gray, blurImage, edge1, cedge;
const char* window_name1 = "Edge map : Canny default (Sobel gradient)";
// define a trackbar callback
static void onTrackbar(int, void*)
{
    blur(gray, blurImage, Size(3,3));
    // Run the edge detector on grayscale
    Canny(blurImage, edge1, edgeThresh, edgeThresh*3, 3);
    cedge = Scalar(255, 0, 0);
    image.copyTo(cedge.colRange(0, image.cols), edge1);
    
    Mat img = image.clone();
	vector<Vec3f> circles;
    HoughCircles(gray, circles, HOUGH_GRADIENT, 2, gray.rows/4, edgeThresh*3 + 3, edgeThresh + 1, gray.rows / 12);
    for (size_t i = 0; i < circles.size(); i++) {
         Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
         int radius = cvRound(circles[i][2]);
         // draw the circle center
         circle(img, center, 2, Scalar(0,255,0), -1, 8, 0);
         // draw the circle outline
         circle(img, center, radius, Scalar(0,0,255), 1, 8, 0);
    }
    img.copyTo(cedge.colRange(img.cols, 2 * img.cols));
    imshow(window_name1, cedge);
}

int main( int argc, const char** argv )
{
    string filename(argv[1]);
    image = imread(filename, 1);
    if(image.empty()) {
        printf("Cannot read image file: %s\n", filename.c_str());
        return 1;
    }
    cedge.create(Size(std::max(300, 2*image.cols), image.rows), image.type());
    cvtColor(image, gray, COLOR_BGR2GRAY);
    // Create a window
    namedWindow(window_name1, 1);
    // create a toolbar
    createTrackbar("T", window_name1, &edgeThresh, 100, onTrackbar);
    // Show the image
    onTrackbar(0, 0);
    // Wait for a key stroke; the same function arranges events processing
    waitKey(0);
    return 0;
}
