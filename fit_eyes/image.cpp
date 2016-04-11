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

template<int N, int M>
void copyToCol(const Vector3 &v, cv::Matx<float, N, M> &m, int column)
{
    // I hate you for having to do this, OpenCV.
    for (int i=0; i<N; i++) {
        m(i, column) = v[i];
    }
}

void sobel(const Bitmap3 &src, Bitmap3 &dst, int dx, int dy)
{
    const int ksize = 3;
    const int type = CV_32FC1;
    Bitmap1 tmp[3];
    cv::split(src, tmp);
    for (int i=0; i<3; i++) {
        cv::Sobel(tmp[i], tmp[i], type, dx, dy, ksize, 1.0 / (1 << (2 * ksize - 2 - dx - dy)));
        //double min = 0, max = 0;
        //cv::minMaxIdx(tmp[i], &min, &max);
        //printf("sobel min %g, max %g\n", min, max);
    }
    cv::merge(tmp, 3, dst);
}

Image::Image()
{
}

Image::Image(const Image &img)
{
    data = img.data.clone();
    for (size_t i=0; i<derivatives.size(); i++) {
        if (not img.derivatives[i].empty()) {
            derivatives[i] = img.derivatives[i].clone();
        }
    }
}

bool Image::read(VideoCapture &cap)
{
    cv::Mat tmp;
    bool result = cap.read(tmp);
    if (result) {
        tmp.convertTo(data, data.type(), 1./255);
        cv::pyrDown(data, data);
        for (Bitmap3 &bm : derivatives) {
            bm = Bitmap3();
        }
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
    copyToCol(d(Order::X, v), result, 0);
    copyToCol(d(Order::Y, v), result, 1);
    assert (result(0, 0) == d(Order::X, v)(0));
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
