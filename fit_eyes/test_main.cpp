#include "main.h"
#include "bitmap.h"

Vector3 pow2(cv::Matx<float, 3, 1> a, cv::Matx<float, 3, 1> b)
{
    Vector3 result;
    for (int i=0; i<3; i++) {
        result(i) = pow2(a(i, 0)) + pow2(b(i, 0));
    }
    return result;
}

int main()
{
    VideoCapture cam{0};
    Bitmap3 img;
    img.read(cam);
    Rect region(200, 200, 50, 50);
    Transformation rot(region);
    rot.params(3) = 0.8;
    Transformation rot_inv = rot.inverse();
    Bitmap3 result(region);
    Rect rotregion = rot(region);
    Bitmap32 grad = gradient(img, rotregion);
    for (Pixel p : Iterrect(rotregion)) {
        Pixel t = to_pixel(rot_inv(p));
        if (region.contains(t)) {
            result(p) = pow2(grad(t).col(0), grad(t).col(1));
        }
    }
    cv::imshow("rotated", result);
    cv::waitKey();
}
