#ifndef BITMAP_H
#define BITMAP_H

#include "main.h"

template<typename T>
class Bitmap : public cv::Mat_<T>
{
    Pixel offset;
    using DataType = cv::Mat_<T>;
    inline int crop_vertical(float y) const;
    inline int crop_horizontal(float x) const;
    inline T sample(Vector2) const;
public:
    const static int halfpixels;
    Bitmap(const DataType &data=DataType(), Pixel offset={0, 0});
    Bitmap(int rows, int cols, T value, Pixel offset={0, 0});
    Bitmap(Rect);
    Bitmap<T> clone() const;
    
    using DataType::operator=;
    
    T operator () (Vector2 pos) const;
    T& operator () (Pixel pos);
    const T& operator () (Pixel pos) const;
    
    bool read(VideoCapture &cap);

    Rect region() const;
};

using Bitmap1 = Bitmap<float>;
using Bitmap2 = Bitmap<Vector2>;
using Bitmap3 = Bitmap<Vector3>;
using Bitmap22 = Bitmap<Matrix22>;
using Bitmap32 = Bitmap<Matrix32>;

Bitmap32 gradient(const Bitmap3&, Rect region=Rect());
Bitmap2 gradient(const Bitmap1&, Rect region=Rect());
Bitmap22 gradient(const Bitmap2&, Rect region=Rect());
Bitmap1 grayscale(const Bitmap3&, Rect region=Rect());

#endif
