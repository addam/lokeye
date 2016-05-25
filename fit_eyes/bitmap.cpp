#include "bitmap.h"

template<typename T>
int Bitmap<T>::crop_vertical(float y) const
{
    const float bottom = offset.y + DataType::rows;
    return (y < bottom) ? (y < offset.y) ? offset.y : int(y) : bottom;
}

template<typename T>
int Bitmap<T>::crop_horizontal(float x) const
{
    const float right = offset.x + DataType::cols;
    return (x < right) ? (x < offset.x) ? offset.x : int(x) : right;
}    

template<typename T>
Bitmap<T>::Bitmap(DataType data, Pixel offset) : DataType{data}, offset{offset}
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
T combine(T x, T y)
{
    throw std::logic_error("not implemented (absurd)");
}

Vector2 combine(Vector2 x, Vector2 y)
{
    return Vector2(x(0), y(0));
}

Matrix22 combine(Matrix22 x, Matrix22 y)
{
    return Matrix22(x(0, 0), y(0, 1), x(1, 0), y(1, 1));
}

Matrix32 combine(Matrix32 x, Matrix32 y)
{
    return Matrix32(x(0, 0), y(0, 1), x(1, 0), y(1, 1), x(2, 0), y(2, 1));
}

template<typename T>
T Bitmap<T>::operator () (Vector2 pos, bool use_half_shift) const
{
    assert (use_half_shift <= half_shift);
    if (use_half_shift) {
        auto &self = *this;
        return combine(self(pos - Vector2(0.5, 0), false), self(pos - Vector2(0, 0.5), false));
    } else {
        float x = pos(0) - offset.x, y = pos(1) - offset.y;
        const T* top = DataType::template ptr<T>(crop_vertical(std::floor(y)));
        const T* bottom = DataType::template ptr<T>(crop_vertical(std::ceil(y)));
        int left = crop_horizontal(std::floor(x)), right = crop_horizontal(std::ceil(x));
        float tb = y - int(y), lr = x - int(x);
        return (1 - tb) * ((1 - lr) * top[left] + lr * top[right]) + tb * ((1 - lr) * bottom[left] + lr * bottom[right]);
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
Rect Bitmap<T>::region() const
{
    return Rect{offset, DataType::size()};
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

template<int N, int M, typename T, typename R>
R gradient(const T &src, Rect region)
{
    init_region(region, src);
    const Pixel delta[] = {{1, 0}, {0, 1}};
    region.width -= 1;
    region.height -= 1;
    if (T::half_shift) {
        region.x += 1;
        region.y += 1;
    }
    R result(region);
    //static_assert (R::half_shift != T::half_shift, "gradient must cause halfpixel shift");
    for (Pixel p : Iterrect(region)) {
        for (int i=0; i<N; i++) {
            for (int j=0; j<M; j++) {
                elem(result(p), i, j) = elem(src(p + delta[j]), i, 0) - elem(src(p), i, 0);
            }
        }
    }
    return result;
}

Bitmap32 gradient(const Bitmap3 &src, Rect region)
{
    return gradient<3, 2, Bitmap3, Bitmap32>(src, region);
}

Bitmap2 gradient(const Bitmap1 &src, Rect region)
{
    return gradient<1, 2, Bitmap1, Bitmap2>(src, region);
}

Bitmap22 gradient(const Bitmap2 &src, Rect region)
{
    return gradient<2, 2, Bitmap2, Bitmap22>(src, region);
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

template<> const bool Bitmap1::half_shift = false;
template<> const bool Bitmap2::half_shift = true;
template<> const bool Bitmap3::half_shift = false;
template<> const bool Bitmap22::half_shift = true;
template<> const bool Bitmap32::half_shift = true;

template class Bitmap<float>;
template class Bitmap<Vector2>;
template class Bitmap<Vector3>;
template class Bitmap<Matrix22>;
template class Bitmap<Matrix32>;
