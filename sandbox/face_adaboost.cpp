#include <opencv2/objdetect.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
using namespace std;
using namespace cv;

String window_name = "face detection";

void detectAndDisplay(Mat &frame, CascadeClassifier &facecl, CascadeClassifier &eyescl)
{
    Mat frame_gray;
    cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
    equalizeHist(frame_gray, frame_gray);

    std::vector<Rect> faces;
    facecl.detectMultiScale(frame_gray, faces, 1.1, 2, CASCADE_SCALE_IMAGE, Size(30, 30));
    for (Rect face : faces) {
        rectangle(frame, face, Scalar(255, 0, 255));
        std::vector<Rect> eyes;
        eyescl.detectMultiScale(frame_gray(face), eyes, 1.1, 2, CASCADE_SCALE_IMAGE, Size(30, 30));
        for (Rect eye : eyes) {
            rectangle(frame, eye + face.tl(), Scalar(255, 0, 0));
        }
    }
    imshow(window_name, frame);
}

int main( int argc, const char** argv )
{
    const std::string face_cascade_name = "/usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml";
    const std::string eyes_cascade_name = "/usr/local/share/OpenCV/haarcascades/haarcascade_eye_tree_eyeglasses.xml";
    CascadeClassifier face_cascade(face_cascade_name), eyes_cascade(eyes_cascade_name);
    VideoCapture capture(0);
    Mat frame;
    while (capture.read(frame)) {
        assert (not frame.empty());
        detectAndDisplay(frame, face_cascade, eyes_cascade);
        if (char(waitKey(10)) == 27) {
            break;
        }
    }
    return 0;
}

