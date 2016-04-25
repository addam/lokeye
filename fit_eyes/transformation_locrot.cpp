#include "main.h"
Transformation::Transformation(Rect region):
    static_params(region.x + region.width / 2, region.y + region.height / 2, std::sqrt(pow2(region.width) + pow2(region.height))),
    params(static_params[0], static_params[1], 0)
{
}

Vector2 Transformation::operator () (Vector2 v) const
{
    float angle = 3 * params[2] / static_params[2], s = sin(angle), c = cos(angle);
    Matrix2 rot = {c, -s, s, c};
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

Vector2 Transformation::inverse(Vector2 v) const
{
    float angle = 3 * params[2] / static_params[2], s = sin(angle), c = cos(angle);
    Matrix2 rot = {c, s, -s, c};
    v = Vector2(v[0] - params[0], v[1] - params[1]);
    return Vector2(static_params[0], static_params[1]) + rot * v;    
}
