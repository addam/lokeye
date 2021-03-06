#include "main.h"
#include "transformation_locrot.h"

using Params = Transformation::Params;

inline Vector2 extract_translation(Params p)
{
    return Vector2(p[0], p[1]);
}

inline float extract_angle(Params p)
{
    return p[2];
}

Transformation::Transformation():
    region(0, 0, 1, 1),
    static_params(Vector2(0, 0), 1),
    params(Vector2(0, 0), 1)
{
}

Transformation::Transformation(Region region):
    region(region),
    static_params(center(region), cv::norm(region.br() - region.tl())),
    params(static_params.first, 0)
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
    params.second = static_params.second * other.params.second / other.static_params.second;
    return *this;
}

Transformation Transformation::operator + (const Params &delta) const
{
    Transformation result(*this);
    result += delta;
    return result;
}

Transformation& Transformation::operator += (const Params &delta)
{
    params.first += extract_translation(delta);
    params.second += extract_angle(delta);
    return *this;
}

Vector2 Transformation::operator () (Vector2 v) const
{
    float s, c;
    sincos(s, c);
    Matrix22 rot = {c, -s, s, c};
    return params.first + rot * (v - static_params.first);
}

Params Transformation::d(Vector2 v, int direction) const
{
    float s, c;
    float coef = sincos(s, c);
    v -= static_params.first;
    return (direction == 0) ? Params{1, 0, coef * (-v[0] * s - v[1] * c)} : Params{0, 1, coef * (v[0] * c - v[1] * s)};
}

Region Transformation::operator () (Region r) const
{
    const Transformation &self = *this;
    Vector2 tl = r.tl(), br = r.br(), bl = {tl(0), br(1)}, tr = {br(0), tl(1)};
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
    return 1;
}

Vector2 Transformation::operator - (const Transformation &other) const
{
    return (params.first - static_params.first) - (other.params.first - other.static_params.first);
}

Transformation Transformation::inverse() const
{
    Region tsf_region = (*this)(region);
    decltype(static_params) inverse_static_params(params.first, static_params.second);
    decltype(params) inverse_params(static_params.first, -params.second);
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

float Transformation::sincos(float &s, float &c) const
{
    float coef = M_PI / static_params.second;
    float angle = coef * params.second;
    s = sin(angle);
    c = cos(angle);
    return coef;
}
