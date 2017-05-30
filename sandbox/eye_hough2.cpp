#include "../src/main.h"
#include "../src/bitmap.h"
#include "subpixel.hpp"

Bitmap3 img;
float pow3(float x)
{
	return x * x * x;
}

void cast_vote(Bitmap1 &img, Vector2 v, float weight)
{
    Vector2 pos = img.to_local(v);
    const int left = pos(0), right = left + 1, y = pos(1);
    if (left < 0 or right >= img.cols or y < 0 or y >= img.rows) {
        return;
    }
    float* row = img.ptr<float>(y);
    const float lr = pos(0) - left;
    row[left] += weight * (1 - lr);
    row[right] += weight * lr;
}

Bitmap1 circles(const Bitmap1 &gray)
{
	// curvature = - (f_xx * f_y**2 + f_yy * f_x**2 - 2*f_xy*f_x*f_y) / (f_x**2 + f_y**2)**(3/2)
    Bitmap1 votes = gray.clone();
    votes = 0;
    Bitmap1 dx = gray.d(0), dy = gray.d(1);
    Bitmap1 dxx = dx.d(0), dxy = dx.d(1), dyy = dy.d(1);
    for (Pixel p : gray) {
        Vector2 v = gray.to_world(p);
        Vector2 gradient(dx(v), dy(v));
        float size = cv::norm(gradient);
        if (size != 0) {
	        float radius = - pow3(size) / (dxx(v) * pow2(gradient[1]) + dyy(v) * pow2(gradient[0]) - 2 * dxy(v) * gradient[0] * gradient[1]);
	        if (radius < 0) {
		        cast_vote(votes, v + radius * gradient / size, size);
		        const int grid = 30;
		        if (std::abs(radius) < 1000 and p.x % grid == 0 and p.y % grid == 0) {
					//if (radius != 0) {
						//printf("%i, %i radius: %g -(%g * %g**2 + %g * %g**2 - 2 * %g * %g * %g / pow3(%g)\n", p.x, p.y, radius, dxx(v), gradient[1], dyy(v), gradient[0], dxy(v), gradient[0], gradient[1], size);
					//}
					cv::line(img, p, to_pixel(gray.to_local(v + radius * gradient / size)), cv::Scalar(0, 255, 0));
					cv::line(img, p, to_pixel(gray.to_local(v - 2 * gradient / size)), cv::Scalar(0, 0, 255));
				}
			}
		}
    }
    Pixel result;
    cv::minMaxLoc(votes, NULL, NULL, NULL, &result);
    cv::circle(img, result, 10, cv::Scalar(0, 255, 255), 3);
    return votes;
}

int main(int argc, char **argv)
{
	if (argc > 1) {
		img.read(argv[1]);
	} else {
	    VideoCapture cam = (argc > 1) ? VideoCapture{argv[1]} : VideoCapture{0};
		img.read(cam);
	}
    cv::blur(img, img, cv::Size(5, 5));
	cv::Mat canvas;
	if (argc == 4 && std::string(argv[2]) == "-q") {
		float radius = atof(argv[3]);
		float x, y;
		find_maximum(circles(img.grayscale()), x, y);
		printf("%.2f %.2f\n", x, y);
		return 0;
	}
	cv::log(circles(img.grayscale()) + 1, canvas);
	cv::imshow("centers", canvas);
	cv::imshow("original", img);
	cv::waitKey();
}
