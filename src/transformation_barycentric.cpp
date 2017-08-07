#include "main.h"
#include "transformation_barycentric.h"

using Params = Transformation::Params;

inline Triangle triangle(Region r)
{
    return {Vector2{r.x, r.y}, Vector2{r.x + r.width/2, r.y + r.height}, Vector2{r.x + r.width, r.y}};
}

inline Params to_matrix(Triangle t)
{
	Matrix23 result;
    for (int j=0; j<t.size(); ++j) {
        set_col(result, j, t[j]);
    }
    return result;
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

inline Matrix23 hmerge(const Matrix22 &l, const Matrix21 &r)
{
	Matrix23 result;
	for (int i=0; i<2; ++i) {
		for (int j=0; j<2; ++j) {
			result(i, j) = l(i, j);
		}
		result(i, 2) = r(i, 0);
	}
	return result;
}

inline Matrix23 dehomogenize(const Matrix33 &mat)
{
	return mat.get_minor<2, 3>(0, 0);
	// TODO: assert something is unity?
}

Vector2 extract_point(const Transformation &tsf, unsigned index)
{
    assert (index < 3);
    return vectorize(tsf.points.col(index));
}

Vector2 extract_point(const Params &p, unsigned index)
{
    assert (index < 3);
    return vectorize(p.col(index));
}

Transformation::Transformation()
{
}

Transformation::Transformation(Region in_region):
    region(triangle(in_region)),
	points(to_matrix(region)),
	static_params(homogenize(points).inv())
{
	update_params();
}

Transformation::Transformation(Triangle in_region):
    region(in_region),
	points(to_matrix(region)),
	static_params(homogenize(points).inv())
{
	update_params();
}

Transformation::Transformation(Triangle region, decltype(params) params, decltype(static_params) static_params):
    region(region),
    static_params(static_params),
    params(params)
{
	points = params * static_params.inv();
}

Transformation& Transformation::operator = (const Transformation &other)
{
	points = dehomogenize(homogenize(other.points) * other.static_params * static_params.inv());
	return *this;
}

void Transformation::update_params()
{
	params = points * static_params;
}

Transformation Transformation::operator + (const Params &delta) const
{
    Transformation result(*this);
    result += delta;
    return result;
}

Transformation& Transformation::operator += (const Params &delta)
{
	points += delta;
	update_params();
    return *this;
}

Transformation& Transformation::increment(Vector2 delta, int index)
{
    add_col(points, index, delta);
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

Triangle Transformation::operator () (Triangle r) const
{
    Triangle result;
    std::transform(r.begin(), r.end(), result.begin(), *this);
    return result;
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
    Triangle tsf_region = (*this)(region);
	Matrix22 invmat = params.get_minor<2, 2>(0, 0).inv();
	return Transformation(tsf_region, hmerge(invmat, -invmat * params.get_minor<2, 1>(0, 2)), homogenize(points).inv());
}

Triangle Transformation::vertices() const
{
    return region;
}
