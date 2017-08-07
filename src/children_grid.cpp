#include "children_grid.h"
#include "optimization.h"

#if defined TRANSFORMATION_PERSPECTIVE_H
const std::array<int, 4> centerpoint_index = {2, 3, 1, 0};
#elif defined TRANSFORMATION_BARYCENTRIC_H
const std::array<int, 6> centerpoint_index = {0, 0, 0, 0, 0, 0};
#endif

Children::Children(const Bitmap3 &image, Region parent):
    ref(image),
    parent_region(parent)
{
    const int divisions = 2;
#if defined TRANSFORMATION_PERSPECTIVE_H
    const float x = parent.x, y = parent.y, w = parent.width / divisions, h = parent.height / divisions;
    for (int i=0; i<divisions; ++i) {
        for (int j=0; j<divisions; ++j) {
            Region r(x + j * w, y + i * h, w, h);
            children.emplace_back(r);
        }
    }
#elif defined TRANSFORMATION_BARYCENTRIC_H
    const float x = parent.x, y = parent.y, w = parent.width, h = parent.height;
    Vector2 center(x+w/2, y+h/2);
    std::array<Vector2, 6> circle = {{{x, y+h/2}, {x+w/3, y}, {x+2*w/3, y}, {x+w, y+h/2}, {x+2*w/3, y+h}, {x+w/3, y+h}}};
    Vector2 prev = circle.back();
    for (Vector2 next : circle) {
        children.emplace_back(Triangle{center, prev, next});
        prev = next;
    }    
#else
#error "Grid is only supported with barycentric or perspective motion model."
#endif
}

void Children::refit(const Bitmap3 &img, const Transformation &parent_tsf)
{
    const int iteration_count = 2;
    const int min_size = 10;
    for (Transformation &tsf : children) {
        tsf = parent_tsf;
    }
    vector<std::pair<Bitmap3, Bitmap3>> pyramid = {{img, ref}};
    for (float size=radius(parent_tsf.region); size > min_size; size /= 2) {
        auto &pair = pyramid.back();
        pyramid.emplace_back(pair.first.downscale(), pair.second.downscale());
    }
    std::reverse(pyramid.begin(), pyramid.end());
    for (const auto &pair : pyramid) {
        Bitmap3 dx = pair.first.d(0), dy = pair.first.d(1);
        vector<float> prev_energy;
        std::transform(children.begin(), children.end(), std::back_inserter(prev_energy), [pair](const Transformation &tsf) { return evaluate(tsf, pair.first, pair.second); });
        for (int iteration=0; iteration < iteration_count; ++iteration) {
            Vector2 delta_center;
            for (int i=0; i<children.size(); ++i) {
                const Transformation &tsf = children[i];
                Transformation::Params delta_tsf = update_step(tsf, pair.first, dx, pair.second, 0) + update_step(tsf, pair.first, dy, pair.second, 1);
                float step_mag = 2 * step_length(delta_tsf, tsf);
                if (step_mag < 1e-10) {
                    break;
                }
                float length = line_search(delta_tsf, prev_energy[i], pair.first.scale / step_mag, tsf, pair.first, pair.second);
                delta_center += length * extract_point(delta_tsf, centerpoint_index[i]);
            }
            for (int i=0; i<children.size(); ++i) {
                children[i].increment(delta_center, centerpoint_index[i]);
            }
        }
    }
}

Vector2 Children::operator()(const Transformation &parent_tsf) const
{
    Vector2 center = extract_point(children.front(), centerpoint_index.front());
    return parent_tsf.inverse(center);
}
