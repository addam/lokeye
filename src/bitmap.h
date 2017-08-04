#ifndef BITMAP_H
#define BITMAP_H

#include "main.h"

/** A wrapper to the OpenCV "typed matrix" class
 * OpenCV supports constant-time cropping of bitmaps
 * but one has to keep track of the coordinate frame.
 * This class manages the coordinates properly, even when scaling and calculating derivatives.
 */
template<typename T>
class Bitmap : public cv::Mat_<T>
{
protected:
    using DataType = cv::Mat_<T>;
    
    /** Position of top left corner in world reference frame
     */
    Vector2 offset;
    
    /** Size of a pixel in world reference frame
     */
    float scale;
public:
    Vector2 to_local(Vector2) const;
    Pixel to_clamped_local(Vector2) const;
    Vector2 to_world(Pixel) const;
    Region to_local(Region) const;
    Region to_world(Rect) const;
    Bitmap(DataType data=DataType(), Vector2 offset={0, 0}, float scale=1);
    Bitmap(int rows, int cols, Vector2 offset={0, 0}, float scale=1);
    Bitmap(Rect, float scale=1);
    Bitmap<T> clone() const;
    
    using DataType::operator=;
    
    /** Access a pixel directly, in the reference frame of this bitmap
     */
    using DataType::operator ();
    
    /** Sample from this bitmap in world reference frame
     */
    T operator () (Vector2) const;
    
    bool contains(Vector2) const;
    
    bool read(VideoCapture&, bool synchronize=false);
    bool read(const string&);

    Bitmap<T> crop(Region) const;
    Bitmap<T> d(int direction, Rect rect=Rect()) const;
    Bitmap<T> d(int direction, Region) const;
    Bitmap<T> downscale() const;
    Bitmap<float> grayscale(Rect rect=Rect()) const;
    Bitmap<float> grayscale(Region) const;
};

struct RectSampling
{
    Pixel tl;
    Pixel br;
    struct Iterator : public Pixel {
        int left;
        int right;
        Iterator(Pixel init, Pixel br): Pixel(init), left(init.x), right(br.x) {
        }
        Iterator(int end): Pixel(0, end) {
        }
        Pixel& operator *() {
            return *this;
        }
        bool operator != (const Pixel &other) const {
            return y != other.y;
        }
        Iterator& operator++() {
            (++x < right) or (++y, x = left);
            return *this;
        }
    };
    Iterator begin() const {
        return Iterator(tl, br);
    }
    Iterator end() const {
        return Iterator(br.y);
    }
};

struct TriSampling
{
    Pixel top, mid, bottom;
    struct Iterator : public Pixel {
        int vert_split;
        std::array<Pixel, 2> ends;
        std::array<Pixel, 2> mids;
        Iterator(Pixel top, Pixel mid, Pixel bottom): Pixel(top), ends{top, bottom}, vert_split(mid.y) {
            mids[0] = mid;
            float t = (bottom.y != top.y) ? float(mid.y - top.y) / (bottom.y - top.y) : 0;
            mids[1] = Pixel(t * bottom.x + (1 - t) * top.x, mid.y);
            if (mids[1].x < mids[0].x) {
                std::swap(mids[0], mids[1]);
            }
        }
        Iterator(int end): Pixel(0, end) {
        }
        Pixel& operator *() {
            return *this;
        }
        bool operator != (const Iterator &other) const {
            return y != other.y;
        }
        Iterator& operator++() {
            (++x < bound(true)) or (++y, x = bound(false));
            return *this;
        }
        float slope2(Pixel a, Pixel b) {
            return float(b.x - a.x) / (b.y - a.y);
        }
        float bound(bool is_right) {
            const Pixel &mid = mids[is_right ? 1 : 0];
            if (y < vert_split) {
                float t = (ends[0].y != mid.y) ? float(y - ends[0].y) / (mid.y - ends[0].y) : 0;
                return t * mid.x + (1 - t) * ends[0].x;
            } else {
                float t = (ends[1].y != mid.y) ? float(y - mid.y) / (ends[1].y - mid.y) : 0;
                return t * ends[1].x + (1 - t) * mid.x;
            }
        }
    };
    Iterator begin() const {
        return Iterator(top, mid, bottom);
    }
    Iterator end() const {
        return Iterator(bottom.y);
    }
};

template<typename T>
RectSampling sampling(const Bitmap<T> &img)
{
    Pixel end(img.cols, img.rows);
    if (end.x == 0) {
        end.y = 0;
    }
    return RectSampling{Pixel(0, 0), end};
}

template<typename T>
RectSampling sampling(const Bitmap<T> &img, const Region &r)
{
    Pixel begin = img.to_clamped_local(r.tl());
    Pixel end = img.to_clamped_local(r.br()) + Pixel(0, 1);
    if (end.x == begin.x) {
        end.y = begin.y;
    }
    return RectSampling{begin, end};
}

template<typename T>
TriSampling sampling(const Bitmap<T> &img, const Triangle &r)
{
    using std::swap;
    Pixel top = img.to_clamped_local(r[0]);
    Pixel mid = img.to_clamped_local(r[1]);
    Pixel bottom = img.to_clamped_local(r[2]);
    if (bottom.y < mid.y) {
        swap(bottom, mid);
    }
    if (mid.y < top.y) {
        swap(mid, top);
    }
    if (bottom.y < mid.y) {
        swap(bottom, mid);
    }
    return TriSampling{top, mid, bottom};
}


using Bitmap1 = Bitmap<float>;
using Bitmap2 = Bitmap<Vector2>;
using Bitmap3 = Bitmap<Vector3>;


#endif
