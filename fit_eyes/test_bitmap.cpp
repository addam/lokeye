#include "bitmap.h"
#include <iostream>

int main()
{
    Bitmap1 gray(4, 4, 0.5);
    for (Pixel p : gray.region()) {
        gray(p) = (p.x + p.y) % 2;
    }
    Bitmap2 d1_img1 = gradient(gray);
    Bitmap22 d2_img1 = gradient(d1_img1);
    for (int i=0; i<=2*gray.rows; i++) {
        for (int j=0; j<=2*gray.cols; j++) {
            Vector2 pos(j/2.f - 0.5, i/2.f - 0.5);
            float val = gray(pos);
            Vector2 d1 = d1_img1(pos);
            Matrix22 d2 = d2_img1(pos);
            printf("@%3.1f, %3.1f: %+4.2f;  dx %+4.2f, dy %+4.2f;  dxx %+4.2f, dxy %+4.2f, dyx %+4.2f, dyy %+4.2f\n", pos(0), pos(1), val, d1(0), d1(1), d2(0, 0), d2(0, 1), d2(1,0), d2(1, 1));
        }
        printf("\n");
    }
    std::cout << "orig data:" << std::endl << gray << std::endl;
    std::cout << "gradient data:" << std::endl << d1_img1 << std::endl;
    std::cout << "jacobian data:" << std::endl << d2_img1 << std::endl;
    Bitmap3 img;
    VideoCapture cam(0);
    img.read(cam);
    Bitmap32 d1_img3 = gradient(img);
    gray = grayscale(img);
}
