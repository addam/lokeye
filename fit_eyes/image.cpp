#include "main.h"
#define RETCASE(x) case x: return #x; break

const char* cvtoa(int type)
{
    switch(type) {
        RETCASE(CV_8UC1);
        RETCASE(CV_8UC3);
        RETCASE(CV_32FC1);
        RETCASE(CV_32FC3);
        RETCASE(CV_64FC1);
        RETCASE(CV_64FC3);
    }
    return "unknown type";
}

void sobel(const Bitmap3 &src, Bitmap3 &dst, int dx, int dy)
{
    printf("sobel\n");
    const int ksize = 3;
    const int type = CV_32FC1;
    Bitmap1 tmp[3];
    double min = 0, max = 0;
    cv::split(src, tmp);
    for (int i=0; i<3; i++) {
        cv::Sobel(tmp[i], tmp[i], type, dx, dy, ksize, 1.0 / (1 << (2 * ksize - 3)));
        cv::minMaxIdx(tmp[i], &min, &max);
        printf("min %g, max %g\n", min, max);
    }
    dst = Bitmap3(src.size());
    cv::merge(tmp, 3, dst);
}

bool Image::read(VideoCapture &cap)
{
    cv::Mat tmp;
    bool result = cap.read(tmp);
    if (result) {
        printf("convert to %s\n", cvtoa(data.type()));
        tmp.convertTo(data, data.type(), 1./255);
    }
    return result;
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
    Matrix32 result;
    result.col(0) = d(Order::X, v);
    result.col(1) = d(Order::Y, v);
    return result;
}

Bitmap3& Image::d(Order order)
{
    assert (not data.empty());
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
