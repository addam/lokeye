#ifndef BITMAP_H
#define BITMAP_H

#include "main.h"

template<typename T>
class Bitmap : public cv::Mat_<T>
{
    using DataType = cv::Mat_<T>;
    Vector2 offset;
public:
    Vector2 to_local(Vector2) const;
    Vector2 to_world(Pixel) const;
    Region to_local(Region) const;
    Region to_world(Rect) const;
    Bitmap(const DataType &data=DataType(), Vector2 offset={0, 0});
    Bitmap(int rows, int cols, Vector2 offset={0, 0});
    Bitmap(Rect);
    Bitmap<T> clone() const;
    
    using DataType::operator=;
    
    /** Access a pixel directly, in the reference frame of this bitmap
     */
    using DataType::operator ();
    
    /** Sample from this bitmap in world reference frame
     */
    T operator () (Vector2 pos) const;
    
    bool read(VideoCapture &cap);

    Bitmap<T> d(int direction, Rect rect=Rect()) const;
    Bitmap<T> d(int direction, Region) const;
    
    struct Iterator : public Pixel {
        int right;
        Iterator(int right, Pixel init={0, 0}) : Pixel{init}, right{right} {
        }
        Pixel& operator *() {
            return *this;
        }
        bool operator != (const Iterator &other) const {
            return not (other == *this);
        }
        Iterator& operator++() {
            if (++x >= right) {
                x = 0;
                ++y;
            }
            return *this;
        }
    };
    Iterator begin() const {
        return Iterator{DataType::cols};
    }
    Iterator end() const {
        return Iterator{DataType::cols, Pixel{0, DataType::rows}};
    }
};

using Bitmap1 = Bitmap<float>;
using Bitmap2 = Bitmap<Vector2>;
using Bitmap3 = Bitmap<Vector3>;

Bitmap1 grayscale(const Bitmap3&, Rect rect=Rect());
Bitmap1 grayscale(const Bitmap3&, Region);

#endif
