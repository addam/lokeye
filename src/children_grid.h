#ifndef CHILDREN_GRID_H
#define CHILDREN_GRID_H
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
    Region parent_region;
    vector<Marker> markers;
};
#endif
