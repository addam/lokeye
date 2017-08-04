#include <opencv2/highgui.hpp>
#include "children_markers.h"
#include "optimization.h"

Children::Children(const Bitmap3 &image, Region parent):
    ref(image)
{
    float scale = parent.height;
    Region upper(parent.x + 0.3*scale, parent.y + 0.25*scale, 0.4*scale, 0.2*scale);
    Region lower(parent.x + 0.3*scale, parent.y + 0.45*scale, 0.4*scale, 0.2*scale);
    children.emplace_back(lower);
    children.emplace_back(upper);
}

void Children::refit(const Bitmap3 &img, const Transformation &parent_tsf)
{
    for (Transformation &tsf : children) {
        refit_transformation(tsf, img, ref);
    }
}

Vector2 Children::operator()(const Transformation &parent_tsf) const
{
    vector<Vector2> centers;
    Transformation inv = parent_tsf.inverse();
    for (const Transformation &tsf : children) {
        centers.emplace_back(inv(tsf(center(tsf.region))));
    }
    return centers.at(1) - centers.at(0);
}
