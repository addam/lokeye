#include "main.h"
Vector2 Transformation::operator () (Vector2 v) const
{
    float angle = params[2], s = sin(angle), c = cos(angle);
    Matrix22 rot = {c, -s, s, c};
    return v + Vector2(params[0], params[1]) + rot * v;
}

Matrix23 Transformation::grad(Vector2 v) const
{
    float angle = params[2], s = sin(angle), c = cos(angle);
    return Matrix23{1, 0, -v[0] * s - v[1] * c, 0, 1, v[0] * c - v[1] * s};
}
