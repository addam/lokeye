#include "main.h"
#include "transformation_affine.h"

using Params = Transformation::Params;

inline Vector2 extract_translation(Params p)
{
    return Vector2(p[0], p[1]);
}

inline Matrix22 extract_matrix(Params p)
{
    return Matrix22(p[2], p[3], p[4], p[5]);
}

Transformation::Transformation():
    region(0, 0, 1, 1),
    static_params(Vector2(0, 0), 1),
    params(std::make_pair(Vector2(0, 0), Matrix22(1, 0, 0, 1)))
{
}

Transformation::Transformation(Region region):
    region(region),
    static_params(center(region), 1. / cv::norm(region.br() - region.tl())),
    params(static_params.first, (1. / static_params.second) * Matrix22::eye())
{
}

Transformation::Transformation(Region region, decltype(params) params, decltype(static_params) static_params):
    region(region),
    static_params(static_params),
    params(params)
{
}

Transformation& Transformation::operator = (const Transformation &other)
{
    Vector2 zero(0, 0);
    params.first += other(zero) - (*this)(zero);
    params.second = (other.static_params.second / static_params.second) * other.params.second;
    return *this;
}

Transformation Transformation::operator + (Params delta) const
{
    Transformation result(*this);
    result += delta;
    return result;
}

Transformation& Transformation::operator += (Params delta)
{
    params.first += extract_translation(delta);
    params.second += extract_matrix(delta);
    return *this;
}

Vector2 Transformation::operator () (Vector2 v) const
{
    return params.first + static_params.second * params.second * (v - static_params.first);
}

Params Transformation::d(Vector2 v, int direction) const
{
    v -= static_params.first;
    float coef = static_params.second;
    return (direction == 0) ? Params{1, 0, coef * v[0], coef * v[1], 0, 0} : Params{0, 1, 0, 0, coef * v[0], coef * v[1]};
}

Region Transformation::operator () (Region region) const
{
    const Transformation &self = *this;
    Vector2 tl = region.tl(), br = region.br(), bl = {tl(0), br(1)}, tr = {br(0), tl(1)};
    Vector2 points[] = {self(tl), self(tr), self(bl), self(br)};
    Vector2 new_tl = points[0], new_br = points[0];
    for (Vector2 v : points) {
        for (int i=0; i<2; i++) {
            new_tl(i) = std::min(new_tl(i), v(i));
            new_br(i) = std::max(new_br(i), v(i));
        }
    }
    return {new_tl, new_br};
}

float Transformation::scale(Vector2) const
{
    return static_params.second * std::sqrt(params.second(0, 0) * params.second(1, 1));
}

Vector2 Transformation::operator - (const Transformation &other) const
{
    return (params.first - static_params.first) - (other.params.first - other.static_params.first);
}

Transformation Transformation::inverse() const
{
    Region tsf_region = (*this)(region);
    decltype(static_params) inverse_static_params(params.first, static_params.second);
    decltype(params) inverse_params(Vector2(static_params.first(0), static_params.first(1)), pow2(1. / static_params.second) * params.second.inv());
    return Transformation(tsf_region, inverse_params, inverse_static_params);
}

Vector2 Transformation::inverse(Vector2 v) const
{
    return inverse()(v);
}

std::array<Vector2, 4> Transformation::vertices() const
{
    const Region &r = region;
    return {r.tl(), Vector2{r.x, r.y + r.height}, r.br(), Vector2{r.x + r.width, r.y}};
}
