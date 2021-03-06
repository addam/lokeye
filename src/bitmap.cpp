#include "bitmap.h"

template<typename T>
inline Vector2 Bitmap<T>::to_local(Vector2 v) const
{
    return (v - offset) / scale;
}

template<typename T>
inline Pixel Bitmap<T>::to_clamped_local(Vector2 v) const
{
    v = to_local(v);
    return Pixel(clamp(v[0], 0, DataType::cols - 1), clamp(v[1], 0, DataType::rows - 1));
}

template<typename T>
inline Vector2 Bitmap<T>::to_world(Pixel p) const
{
    return to_vector(p) * scale + offset;
}

template<typename T>
inline Region Bitmap<T>::to_local(Region r) const
{
    return Region(to_local(r.tl()), to_local(r.br()));
}

template<typename T>
inline Region Bitmap<T>::to_world(Rect r) const
{
    return Region(to_world(r.tl()), to_world(r.br()));
}

template<typename T>
Bitmap<T>::Bitmap(DataType data, Vector2 offset, float scale) : DataType{data}, offset{offset}, scale{scale}
{
}

template<typename T>
Bitmap<T>::Bitmap(int rows, int cols, Vector2 offset, float scale) : DataType{rows, cols}, offset{offset}, scale{scale}
{
}

template<typename T>
Bitmap<T>::Bitmap(Rect rect, float scale) : DataType{rect.size()}, offset{to_vector(rect.tl())}, scale{scale}
{
}

template<typename T>
Bitmap<T> Bitmap<T>::clone() const {
    return Bitmap<T>(static_cast<const DataType&>(*this).clone(), offset, scale);
}

template<typename T>
inline T Bitmap<T>::operator() (Vector2 world_pos) const
{
    Vector2 pos = to_local(world_pos);
    if (scale < 0) { 
        return DataType::template at<T>(int(clamp(pos(1), 0, DataType::rows - 1)), int(clamp(pos(0), 0, DataType::cols - 1)));
    }
    int cols = DataType::cols - 1, rows = DataType::rows - 1;
    const T* top = DataType::template ptr<T>(clamp(pos(1), 0, rows));
    const T* bottom = DataType::template ptr<T>(clamp(pos(1) + 1, 0, rows));
    int left = clamp(pos(0), 0, cols), right = clamp(pos(0) + 1, 0, cols);
    float tb = pos(1) - int(pos(1)), lr = pos(0) - int(pos(0));
    return (1 - tb) * ((1 - lr) * top[left] + lr * top[right]) + tb * ((1 - lr) * bottom[left] + lr * bottom[right]);
}

template<typename T>
inline bool Bitmap<T>::contains(Vector2 world_pos) const
{
    Pixel p = to_pixel(to_local(world_pos));
    return 0 <= p.x and p.x < DataType::cols and 0 <= p.y and p.y < DataType::rows;
}

template<typename T>
Bitmap<T> Bitmap<T>::crop(Region region) const
{
    Rect rect = to_rect(to_local(region)) & Rect(Pixel(0, 0), DataType::size());
    if (not rect.area()) {
        return *this;
    }
    Bitmap<T> result(static_cast<const DataType&>(*this)(rect));
    result.offset = to_world(rect.tl());
    result.scale = scale;
    return result;
}

template<>
bool Bitmap3::read(VideoCapture &cap, bool synchronize)
{
    cv::Mat tmp;
	if (not synchronize or cap.get(cv::CAP_PROP_FRAME_COUNT)) {
		if (not cap.read(tmp)) {
			return false;
		}
	} else {
        for (int i=0; i<30; ++i) {
            // make sure that we got a fresh frame
            TimePoint time_start = std::chrono::high_resolution_clock::now();
            if (not cap.grab()) {
                return false;
            }
            float grabbing_time = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - time_start).count();
            if (grabbing_time * cap.get(cv::CAP_PROP_FPS) > 0.5) {
                break;
            }
        }
        cv::Mat tmp;
        cap.grab();  // apparently, we need to drop one more frame
        cap.retrieve(tmp);
    }
    tmp.convertTo(static_cast<DataType&>(*this), DataType().type(), 1./255);
    scale = 1;
    return true;
}

template<>
bool Bitmap3::read(const string &filename)
{
    cv::Mat tmp = cv::imread(filename, cv::IMREAD_COLOR);
    tmp.convertTo(static_cast<DataType&>(*this), DataType().type(), 1./255);
    scale = 1;
    return true;
}

template<typename T>
void init_rect(Rect &region, T img)
{
    Rect img_region{0, 0, img.cols, img.rows};
    if (region.area() == 0) {
        region = img_region;
    } else {
        region &= img_region;
    }
}

template<typename T>
Bitmap<T> Bitmap<T>::d(int direction, Rect rect) const
{
    init_rect(rect, *this);
    Vector2 region_offset = to_world(rect.tl());
    if (direction == 0) {
        rect.width -= 1;
        region_offset(0) += 0.5 * scale;
    } else if (direction == 1) {
        rect.height -= 1;
        region_offset(1) += 0.5 * scale;
    }
    const Bitmap<T> &self = *this;
    Bitmap<T> result(rect.height, rect.width, region_offset, scale);
    const Pixel delta{direction == 0, direction == 1};
    for (Pixel p : sampling(result)) {
        result(p) = self(p + rect.tl() + delta) - self(p + rect.tl());
    }
    return result;
}

template<typename T>
Bitmap<T> Bitmap<T>::d(int direction, Region region) const
{
    return this->d(direction, to_rect(to_local(region)));
}
    
template<typename T>
Bitmap<T> Bitmap<T>::downscale() const
{
    /// @todo do some filtering...
    Bitmap<T> result(DataType::rows / 2, DataType::cols / 2, offset);
    result.scale = 2 * scale;
    result.offset += Vector2(scale, scale);
    result = 0 * T();
    for (Pixel s : sampling(*this)) {
        Pixel d(s.x / 2, s.y / 2);
        if (d.y < result.rows and d.x < result.cols) {
            result(d) += (*this)(s);
        }
    }
    return result;
}

template<>
Bitmap<float> Bitmap<Vector3>::grayscale(Rect rect) const
{
    init_rect(rect, *this);
    Bitmap1 result(rect, scale);
    const Vector3 coef = {0.114, 0.587, 0.299};
    for (Pixel p : sampling(result)) {
        const Vector3 val = (*this)(p + rect.tl());
        result(p) = val.dot(coef);
    }
    return result;
}

template<>
Bitmap<float> Bitmap<Vector3>::grayscale(Region region) const
{
    return grayscale(to_rect(to_local(region)));
}

template class Bitmap<float>;
template class Bitmap<Vector2>;
template class Bitmap<Vector3>;
