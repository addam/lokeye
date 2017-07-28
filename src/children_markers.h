#ifndef CHILDREN_MARKERS_H
#define CHILDREN_MARKERS_H
#include "bitmap.h"
#include "main.h"
struct Transformation;

struct Children
{
    Children(const Bitmap3&, Region parent);
    void refit(const Bitmap3&, const Transformation&);
    Vector2 operator() (const Transformation&) const;
    void render(Bitmap3&) const;
protected:
    using Marker = std::pair<Transformation, Region>;
    Bitmap3 ref;
    vector<Marker> markers;
};
#endif
