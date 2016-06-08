#ifndef BITMAP_H
#define BITMAP_H

#include "main.h"

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
    T operator () (Vector2 pos) const;
    
    bool read(VideoCapture &cap);

    Bitmap<T> crop(Region) const;
    Bitmap<T> d(int direction, Rect rect=Rect()) const;
    Bitmap<T> d(int direction, Region) const;
    Bitmap<T> downscale() const;
    Bitmap<float> grayscale(Rect rect=Rect()) const;
    Bitmap<float> grayscale(Region) const;
    
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


#endif
