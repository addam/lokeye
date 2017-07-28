#include "children_markers.h"
#include "optimization.h"

Children::Children(const Bitmap3 &image, Region parent):
    ref(image)
{
    float scale = parent.height;
    Region upper(parent.x + 0.4*scale, parent.y + 0.25*scale, 0.2*scale, 0.2*scale);
    Region lower(parent.x + 0.4*scale, parent.y + 0.45*scale, 0.2*scale, 0.2*scale);
    markers.emplace_back(Transformation(), lower);
    markers.emplace_back(Transformation(), upper);
}

void Children::refit(const Bitmap3 &img, const Transformation &parent_tsf)
{
    for (Marker &m : markers) {
        m.first = parent_tsf;
        refit_transformation(m.first, m.second, img, ref);
    }
}

Vector2 Children::operator()(const Transformation &parent_tsf) const
{
    vector<Vector2> centers;
    Transformation inv = parent_tsf.inverse();
    for (const Marker &m : markers) {
        const Region &r = m.second;
        centers.emplace_back(inv(m.first(to_vector(r.tl() + r.br()) / 2)));
    }
    return centers.at(1) - centers.at(0);
}
