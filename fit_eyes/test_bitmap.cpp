#include "bitmap.h"

int main()
{
    VideoCapture cam{0};
    Bitmap3 img;
    img.read(cam);
    auto gray = grayscale(img);
    Bitmap32 d1_img3 = gradient(img);
    Bitmap2 d1_img1 = gradient(gray);
    Bitmap22 d2_img1 = gradient(d1_img1);
}
