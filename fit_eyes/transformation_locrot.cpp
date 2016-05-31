#include "main.h"
Transformation::Transformation(Region region):
    static_params(region.x + region.width / 2, region.y + region.height / 2, std::sqrt(pow2(region.width) + pow2(region.height))),
    params(static_params[0], static_params[1], 0)
{
}

Transformation::Transformation(Params params, Vector3 static_params) : static_params(static_params), params(params)
{
}

Vector2 Transformation::operator () (Vector2 v) const
{
    float angle = 3 * params[2] / static_params[2], s = sin(angle), c = cos(angle);
    Matrix22 rot = {c, -s, s, c};
    v = Vector2(v[0] - static_params[0], v[1] - static_params[1]);
    return Vector2(params[0], params[1]) + rot * v;
}

Matrix23 Transformation::grad(Vector2 v) const
{
    float coef = 3.f / static_params[2];
    float angle = coef * params[2], s = sin(angle), c = cos(angle);
    v = Vector2(v[0] - static_params[0], v[1] - static_params[1]);
    return Matrix23{1, 0, coef * (-v[0] * s - v[1] * c), 0, 1, coef * (v[0] * c - v[1] * s)};
}

Vector3 Transformation::d(Vector2 v, int direction) const
{
    float coef = 3.f / static_params[2];
    float angle = coef * params[2], s = sin(angle), c = cos(angle);
    v = Vector2(v[0] - static_params[0], v[1] - static_params[1]);
    return (direction == 0) ? Vector3{1, 0, coef * (-v[0] * s - v[1] * c)} : Vector3{0, 1, coef * (v[0] * c - v[1] * s)};
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

Transformation Transformation::inverse() const
{
    Vector3 inverse_static_params(params(0), params(1), static_params(2));
    Params inverse_params(static_params(0), static_params(1), -params(2));
    return Transformation(inverse_params, inverse_static_params);
}
