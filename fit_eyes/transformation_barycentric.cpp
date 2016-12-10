#include "main.h"
#include "transformation_barycentric.h"
// TEMPORARY DEBUG
#include <iostream>

using Params = Transformation::Params;

inline Params triangle(Region region)
{
	return Matrix23(region.x, region.x + region.width/2, region.x + region.width,
		region.y, region.y + region.height, region.y);
}

inline Matrix33 homogenize(const Matrix23 &mat)
{
	Matrix33 result = Matrix33::ones();
	for (int i=0; i<2; ++i) {
		for (int j=0; j<3; ++j) {
			result(i, j) = mat(i, j);
		}
	}
	return result;
}

inline Matrix23 dehomogenize(const Matrix33 &mat)
{
	for (int j=0; j<3; ++j) {
		if (std::abs(mat(2, j) - 1) > 1e-6) {
			fprintf(stderr, "dehomogenize error @%i: %g\n", j, mat(2, j));
		}
	}
	return mat.get_minor<2, 3>(0, 0);
	// TODO: assert something is unity?
}

Transformation::Transformation()
{
}

Transformation::Transformation(Region region):
	points(triangle(region)),
	static_params(homogenize(points).inv())
{
	update_params();
}

Transformation::Transformation(decltype(params) params, decltype(static_params) static_params):
    static_params(static_params),
    params(params)
{
	points = params * static_params.inv();
}

Transformation& Transformation::operator = (const Transformation &other)
{
	points = dehomogenize(homogenize(other.points) * other.static_params * static_params.inv());
}

void Transformation::update_params()
{
	params = points * static_params;
}

Transformation Transformation::operator + (Params delta) const
{
    Transformation result(*this);
    result += delta;
    return result;
}

Transformation& Transformation::operator += (Params delta)
{
	points += delta;
	update_params();
    return *this;
}

Vector2 Transformation::operator () (Vector2 v) const
{
	return params * homogenize(v);
}

Params Transformation::d(Vector2 v, int direction) const
{
	Params result = Params::zeros();
	Vector3 row = static_params * homogenize(v);
	for (int j=0; j<3; ++j) {
		result(direction, j) = row(j);
	}
	return result;
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
    return std::sqrt(params(0, 0) * params(1, 1));
}

Vector2 Transformation::operator - (const Transformation &other) const
{
	Vector2 center = vectorize(points.col(0) + points.col(1) + points.col(2)) * (1. / 3);
	return other(center) - (*this)(center);
}

Transformation Transformation::inverse() const
{
	return Transformation(dehomogenize(static_params.inv()), homogenize(params).inv());
}

