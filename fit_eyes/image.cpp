#include "main.h"

void sobel(Bitmap3 src, Bitmap3 dst, int dx, int dy)
{
    const int ksize = 3;
    const int type = CV_32FC1;
    Bitmap1 tmp[3];
    cv::split(src, tmp);
    for (int i=0; i<3; i++) {
        cv::Sobel(tmp[i], tmp[i], type, dx, dy, ksize, 1.0 / (1 << (2 * ksize - 3)));
    }
    cv::merge(tmp, 3, dst);
}

void Image::read(VideoCapture &cap)
{
    cv::Mat tmp;
    cap.read(tmp);
    tmp.convertTo(data, CV_32FC1, 1./255);
}

Iterrect Image::region() const
{
    return Iterrect(data);
}

Color Image::operator () (Pixel p) const {
    /// @todo subsample
    return data(p);
}

Matrix32 Image::grad(Vector2 v)
{
    /// @todo subsample
    Matrix32 result;
    for (int i=0; i<2; i++) {
        if (derivatives[i].empty()) {
            sobel(data, derivatives[i], i==0, i==1);
        }
        result.col(i) = derivatives[i](to_pixel(v));
    }
    return result;
}

Bitmap3& Image::d(Order order)
{
    size_t index = static_cast<size_t>(order);
    if (derivatives[index].empty()) {
        const int dx[] = {1, 0, 2, 1, 0};
        const int dy[] = {0, 1, 0, 1, 2};
        sobel(data, derivatives[index], dx[index], dy[index]);
    }
    return derivatives[index];
}

Color Image::d(Order order, Vector2 v)
{
    /// @todo subsample
    return d(order)(to_pixel(v));
}
