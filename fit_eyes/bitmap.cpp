#include "bitmap.h"

template<typename T>
int Bitmap<T>::crop_vertical(float y) const
{
    const float top = offset.y + 0.5 * halfpixels;
    const float bottom = top + DataType::rows - halfpixels;
    return (y < bottom) ? (y < top) ? top : int(y) : bottom;
}

template<typename T>
int Bitmap<T>::crop_horizontal(float x) const
{
    const float left = offset.x + 0.5 * halfpixels;
    const float right = left + DataType::cols - halfpixels;
    return (x < right) ? (x < left) ? left : int(x) : right;
}    

template<typename T>
Bitmap<T>::Bitmap(const DataType &data, Pixel offset) : DataType{data}, offset{offset}
{
}

template<typename T>
Bitmap<T>::Bitmap(int rows, int cols, T value, Pixel offset) : DataType{rows, cols}, offset{offset}
{
    for (Pixel p : Iterrect(region())) {
        (*this)(p) = value;
    }
}

template<typename T>
Bitmap<T>::Bitmap(Rect region) : DataType{region.size()}, offset{region.tl()}
{
}

template<typename T>
Bitmap<T> Bitmap<T>::clone() const {
    return Bitmap<T>(static_cast<const DataType&>(*this).clone(), offset);
}

template<typename T>
T combine(T x, T y, T w=T())
{
    throw std::logic_error("not implemented (absurd)");
}

Vector2 combine(Vector2 x, Vector2 y)
{
    return Vector2(x(0), y(0));
}

Matrix22 combine(Matrix22 xx, Matrix22 xy, Matrix22 yy)
{
    return Matrix22(xx(0, 0), xy(0, 1), xy(1, 0), yy(1, 1));
}

Matrix32 combine(Matrix32 x, Matrix32 y)
{
    return Matrix32(x(0, 0), y(0, 1), x(1, 0), y(1, 1), x(2, 0), y(2, 1));
}

template<typename T>
inline T Bitmap<T>::sample(Vector2 pos) const
{
    float x = pos(0) - offset.x, y = pos(1) - offset.y;
    const T* top = DataType::template ptr<T>(crop_vertical(y));
    const T* bottom = DataType::template ptr<T>(crop_vertical(y + 1));
    int left = crop_horizontal(x), right = crop_horizontal(x + 1);
    float tb = y - int(y), lr = x - int(x);
    return (1 - tb) * ((1 - lr) * top[left] + lr * top[right]) + tb * ((1 - lr) * bottom[left] + lr * bottom[right]);
}

template<typename T>
T Bitmap<T>::operator () (Vector2 pos) const
{
    switch (halfpixels) {
    case 0:
        return sample(pos);
    case 1:
        return combine(sample(pos - Vector2(0.5, 0)), sample(pos - Vector2(0, 0.5)));
    case 2:
        return combine(sample(pos - Vector2(1, 0)), sample(pos - Vector2(0.5, 0.5)), sample(pos - Vector2(0, 1)));
    default:
        throw std::logic_error("higher derivatives not implemented");
    }
}

template<typename T>
const T& Bitmap<T>::operator () (Pixel pos) const
{
    return static_cast<const DataType&>(*this) (pos - offset);
}
    
template<typename T>
T& Bitmap<T>::operator () (Pixel pos)
{
    return const_cast<T&>(static_cast<const Bitmap<T>&>(*this)(pos));
}
    
template<typename T>
Iterrect Bitmap<T>::region() const
{
    return Rect(offset, DataType::size());
}

template<>
bool Bitmap3::read(VideoCapture &cap)
{
    while (1) {
        // make sure that we got a new image
        TimePoint time_start = std::chrono::high_resolution_clock::now();
        if (not cap.grab()) {
            return false;
        }
        if (std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - time_start).count() * cap.get(cv::CAP_PROP_FPS) > 0.5) {
            break;
        }
    }
    cv::Mat tmp;
    cap.retrieve(tmp);
    tmp.convertTo(static_cast<DataType&>(*this), DataType().type(), 1./255);
    //cv::pyrDown(data, data);
    return true;
}


template<typename T>
void init_region(Rect &region, T img)
{
    if (region.area() == 0) {
        region = img.region();
    } else {
        region &= img.region();
    }
}

template<typename T>
float& elem(T &matrix, int i, int j)
{
    return matrix(i, j);
}

float& elem(Vector2 &matrix, int i, int j)
{
    return matrix(i + j);
}

const float& elem(const Vector2 &matrix, int i, int j)
{
    return matrix(i + j);
}

const float& elem(const Vector3 &matrix, int i, int j)
{
    return matrix(i + j);
}

const float& elem(const float &matrix, int i, int j)
{
    return matrix;
}

template<int N, typename T, typename R>
R gradient(const T &src, Rect region)
{
    init_region(region, src);
    R result(region);
    static_assert (R::halfpixels == T::halfpixels + 1, "gradient must cause halfpixel shift");
    for (Pixel p : Iterrect(region)) {
        for (int i=0; i<N; i++) {
            Pixel dx = {1, 0}, dy = {0, 1};
            elem(result(p), i, 0) = (p.x + 1 < region.br().x) ? elem(src(p + dx), i, 0) - elem(src(p), i, 0) : 0;
            elem(result(p), i, 1) = (p.y + 1 < region.br().y) ? elem(src(p + dy), i, 1) - elem(src(p), i, 1) : 0;
        }
    }
    return result;
}

Bitmap32 gradient(const Bitmap3 &src, Rect region)
{
    return gradient<3, Bitmap3, Bitmap32>(src, region);
}

Bitmap2 gradient(const Bitmap1 &src, Rect region)
{
    return gradient<1, Bitmap1, Bitmap2>(src, region);
}

Bitmap22 gradient(const Bitmap2 &src, Rect region)
{
    return gradient<2, Bitmap2, Bitmap22>(src, region);
}

Bitmap1 grayscale(const Bitmap3 &src, Rect region)
{
    init_region(region, src);
    Bitmap1 result(region);
    const Vector3 coef = {0.114, 0.587, 0.299};
    for (Pixel p : Iterrect(region)) {
        const Vector3 val = src(p);
        result(p) = val.dot(coef);
    }
    return result;
}

template<> const int Bitmap1::halfpixels = 0;
template<> const int Bitmap2::halfpixels = 1;
template<> const int Bitmap3::halfpixels = 0;
template<> const int Bitmap22::halfpixels = 2;
template<> const int Bitmap32::halfpixels = 1;

template class Bitmap<float>;
template class Bitmap<Vector2>;
template class Bitmap<Vector3>;
template class Bitmap<Matrix22>;
template class Bitmap<Matrix32>;
