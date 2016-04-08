#include "main.h"

void sobel(Bitmap src, Bitmap dst, int dx, int dy)
{
    const int ksize = 3;
    const int type = CV_32FC1;
    cv::Mat tmp[3];
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

Vec3f Image::operator () (Vec2f pos) const {
    /// @todo subsample
    return data(round(pos));
}

Matx32f Image::grad(Vec2f pos)
{
    /// @todo subsample
    Matx32f result;
    for (int i=0; i<2; i++) {
        if (derivatives[i].empty()) {
            sobel(data, derivatives[i], i==0, i==1);
        }
        result.col(i) = derivatives[i](round(pos));
    }
    return result;
}

Vec3f Image::d(Order order, Vec2f pos)
{
    /// @todo subsample
    size_t index = static_cast<size_t>(order);
    if (derivatives[index].empty()) {
        const int dx[] = {1, 0, 2, 1, 0};
        const int dy[] = {0, 1, 0, 1, 2};
        sobel(data, derivatives[index], dx[index], dy[index]);
    }
    return derivatives[index](round(pos));
}
