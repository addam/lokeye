#include "bitmap.h"
#include <iostream>

int main()
{
    Bitmap1 gray(4, 4);
    for (Pixel p : gray) {
        gray(p) = (p.x + p.y) % 2;
    }
    Bitmap1 dx = gray.d(0), dy = gray.d(1);
    Bitmap1 dxx = dx.d(0), dxy = dx.d(1), dyx = dy.d(0), dyy = dy.d(1);
    for (int i=0; i<=2*gray.rows; i++) {
        for (int j=0; j<=2*gray.cols; j++) {
            Vector2 v(j/2.f - 0.5, i/2.f - 0.5);
            printf("@%3.1f, %3.1f: %+4.2f;  dx %+4.2f, dy %+4.2f;  dxx %+4.2f, dxy %+4.2f, dyx %+4.2f, dyy %+4.2f\n", v(0), v(1), gray(v), dx(v), dy(v), dxx(v), dxy(v), dyx(v), dyy(v));
        }
        printf("\n");
    }
    std::cout << "orig data:" << std::endl << gray << std::endl;
    std::cout << "gradient data:" << std::endl << dx << std::endl << "-----" << std::endl << dy << std::endl;
    std::cout << "hessian data:" << std::endl << dxx << std::endl << "-----" << std::endl << dxy  << std::endl << "-----" << std::endl << dyx << std::endl << "-----" << std::endl << dyy << std::endl;
    Bitmap3 img;
    VideoCapture cam(0);
    img.read(cam);
    Bitmap3 dx_img3 = img.d(0);
    Bitmap3 dy_img3 = img.d(1);
    gray = img.grayscale();
    gray = gray.downscale();
}
