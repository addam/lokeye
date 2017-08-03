#include <opencv2/highgui.hpp>
#include "children_markers.h"
#include "optimization.h"

Children::Children(const Bitmap3 &image, Region parent):
    ref(image)
{
    float scale = parent.height;
    Region upper(parent.x + 0.3*scale, parent.y + 0.25*scale, 0.4*scale, 0.2*scale);
    Region lower(parent.x + 0.3*scale, parent.y + 0.45*scale, 0.4*scale, 0.2*scale);
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

void Children::render(Bitmap3 &canvas) const
{
    for (const Marker &m : markers) {
        Vector2 tl = to_vector(m.second.tl()), br = to_vector(m.second.br());
        std::array<Vector2, 4> vertices{tl, Vector2(tl[0], br[1]), br, Vector2(br[0], tl[1])};
        std::for_each(vertices.begin(), vertices.end(), [m](Vector2 &v) { v = m.first(v); });
        for (int i = 0; i < 4; i++) {
            cv::line(canvas, to_pixel(vertices[i]), to_pixel(vertices[(i+1)%4]), cv::Scalar(0, 0.8, 1.0));
        }
    }
}
