#include "main.h"
#include "bitmap.h"
#include "transformation_barycentric.h"

int main()
{
    cv::VideoCapture cam(0);
	Bitmap3 img;
	img.read(cam);
	Matrix33 static_params(0.005, 0, -0.5, 0, 0.00578, -0.578, 0, 0, 1);
	Matrix23 params(1, 0, 0, 0, 1, 0), rot(0, 2, -1, -2, 0, 1);
	Transformation tsf(params, static_params);
	cv::imshow("original", img);
	while (char(cv::waitKey(100)) != 27) {
		Bitmap3 tsf_img = img.clone();
		for (Pixel p : tsf_img) {
			tsf_img(p) = img(tsf(p));
		}
		cv::imshow("trasformed", tsf_img);
		tsf += rot;
	}
}
