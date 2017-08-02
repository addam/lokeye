#include <opencv2/highgui.hpp>
#include "children_grid.h"
#include "optimization.h"
#ifndef TRANSFORMATION_PERSPECTIVE_H
#error "Grid is only supported with perspective motion model."
#endif


Children::Children(const Bitmap3 &image, Region parent):
    ref(image),
    parent_region(parent)
{
    const int divisions = 2;
    const float x = parent.x, y = parent.y, w = parent.width / divisions, h = parent.height / divisions;
    for (int i=0; i<divisions; ++i) {
        for (int j=0; j<divisions; ++j) {
            Region r(x + j * w, y + i * h, w, h);
            markers.emplace_back(Transformation(), r);
        }
    }
}

void Children::refit(const Bitmap3 &img, const Transformation &parent_tsf)
{
    const int iteration_count = 2;
    const int min_size = 20000;
    for (Marker &m : markers) {
        m.first = parent_tsf;
    }
    Region rotregion = parent_tsf(parent_region);
    vector<std::pair<Bitmap3, Bitmap3>> pyramid{std::make_pair(img.crop(rotregion), ref.crop(parent_region))};
    if (std::min({pyramid.back().first.rows, pyramid.back().first.cols, pyramid.back().second.rows, pyramid.back().second.cols}) == 0) {
        return;
    }
    while (std::min({pyramid.back().first.rows, pyramid.back().first.cols, pyramid.back().second.rows, pyramid.back().second.cols}) >= 2 * min_size) {
        auto &pair = pyramid.back();
        pyramid.emplace_back(pair.first.downscale(), pair.second.downscale());
    }
    while (not pyramid.empty()) {
        auto &pair = pyramid.back();
        Bitmap3 dx = pair.first.d(0), dy = pair.first.d(1);
        vector<float> prev_energy;
        std::transform(markers.begin(), markers.end(), std::back_inserter(prev_energy), [pair](const Marker &m) { return evaluate(m.first, pair.first, pair.second.crop(m.second)); });
        for (int iteration=0; iteration < iteration_count; ++iteration) {
            /// @todo this does not allow anything more than 2x2 markers
            Vector2 delta_center(0, 0);
            const std::array<int, 4> centerpoint_index = {2, 1, 3, 0};
            for (int i=0; i<markers.size(); ++i) {
                const Marker &m = markers[i];
                const Transformation &tsf = m.first;
                Transformation::Params delta_tsf = update_step(tsf, pair.first, dx.crop(m.second), pair.second, 0) + update_step(tsf, pair.first, dy.crop(m.second), pair.second, 1);
                float max_length = (1 << pyramid.size()) / step_length(delta_tsf, m.second, tsf);
                float length = line_search(delta_tsf, max_length, prev_energy[i], tsf, pair.first, pair.second.crop(m.second));
                delta_center += extract_point(delta_tsf, centerpoint_index[i]) * length;
            }
            delta_center = {1, 1};
            printf("delta center = %g, %g\n", delta_center[0], delta_center[1]);
            for (int i=0; i<markers.size(); ++i) {
                markers[i].first.increment(delta_center, centerpoint_index[i]);            
            }
        }
        pyramid.pop_back();
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
