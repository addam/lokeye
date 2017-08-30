#include "main.h"
#include "optimization.h"
#include "homography.h"
#include "transformation_perspective.h"    

using Params = Transformation::Params;
using PointPack = Transformation::PointPack;

namespace {

Matrix33 canonize(PointPack points)
{
    Matrix33 result;
    for (int i=0; i<3; ++i) {
        Vector3 row = homogenize(points[(i+1) % 3]).cross(homogenize(points[(i+2) % 3]));
        set_row(result, i, row * (1. / row.dot(homogenize(points[3]))));
    }
    return result;
}

Matrix33 decanonize(PointPack points, int derivative=3)
{
    Matrix33 system;
    for (int i=0; i<3; ++i) {
        set_col(system, i, homogenize(points[i]));
    }
    Vector3 last_point = homogenize(points.back());
    if (derivative < 3) {
		for (int i=0; i<3; ++i) {
			last_point(i) = (i == derivative);
		}
	}
    Vector3 alpha = system.solve(last_point, cv::DECOMP_LU);
    for (int i=0; i<3; ++i) {
        set_col(system, i, alpha(i) * homogenize(points[i]));
    }
    return system;
}

vector<std::pair<Vector2, Vector2>> zip_measurements(const PointPack &left, const PointPack &right)
{
	vector<std::pair<Vector2, Vector2>> result;
	for (int i=0; i<4; ++i) {
		result.emplace_back(std::make_pair(left[i], right[i]));
	}
	return result;
}

Params pack_vectors(const array<array<Vector2, 2>, 4> &vec, int direction)
{
    Params result;
    for (int i=0; i<4; ++i) {
		for (int j=0; j<2; ++j) {
	        result[2*i + j] = vec[i][j][direction];
		}
    }
    return result;
}

Vector2 projection_derivative(Vector2 projected_v, float w, Vector3 rhs_factor)
{
    return Vector2(rhs_factor[0] - projected_v[0]*rhs_factor[2], rhs_factor[1] - projected_v[1]*rhs_factor[2]) / w;
}

template<typename T>
void cycle(T &seq)
{
    std::rotate(seq.begin(), seq.begin() + 1, seq.end());
}

array<Matrix33, 4> canonize_all_permutations(PointPack points)
{
    array<Matrix33, 4> result{};
    for (int i=0; i<4; ++i) {
        cycle(points);
        result[i] = canonize(points);
    }
    return result;
}

}

Vector2 extract_point(const Transformation &tsf, unsigned index)
{
    assert (index < 4);
    return tsf.points[index];
}

Vector2 extract_point(const Params &p, unsigned index)
{
    assert (index < 4);
    return Vector2(p[2*index + 0], p[2*index + 1]);
}

Transformation::Transformation():
    region(0, 0, 1, 1),
    static_params{Vector2{0, 0}, Vector2{0, 1}, Vector2{1, 1}, Vector2{1, 0}},
    canonize_matrix(canonize_all_permutations(static_params))
{
    points = static_params;
    update_params(Matrix33::eye());
}

Transformation::Transformation(Region region):
    region(region),
    static_params(extract_points(region)),
    points(static_params),
    canonize_matrix(canonize_all_permutations(static_params))
{
    update_params(Matrix33::eye());
}

void Transformation::update_params(decltype(params) in_params)
{
	// why don't we update `points` here? They depend on `params`, anyway.
    params = in_params;
    for (int i=0; i<4; ++i) {
        cycle(points);
        derivative_matrix[i][0] = decanonize(points, 0);
        derivative_matrix[i][1] = decanonize(points, 1);
        weight_vector[i] = vectorize(decanonize(points).row(2));
    }
}

Transformation::Transformation(Region region, decltype(params) in_params, decltype(static_params) static_params):
    region(region),
    static_params(static_params),
    canonize_matrix(canonize_all_permutations(static_params))
{
    for (int i=0; i<4; ++i) {
        points[i] = project(static_params[i], in_params);
    }
    update_params(in_params);
}

Transformation& Transformation::operator = (const Transformation &other)
{
    for (int i=0; i<4; ++i) {
        points[i] = project(static_params[i], other.params);
    }
    update_params(other.params);
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
    for (int i=0; i<points.size(); ++i) {
        points[i] += extract_point(delta, i);
    }
    update_params(homography<3, 3>(zip_measurements(static_params, points)));
    return *this;
}

Transformation& Transformation::increment(Vector2 delta, int index)
{
    points[index] += delta;
    update_params(homography<3, 3>(zip_measurements(static_params, points)));
    return *this;
}

Vector2 Transformation::operator () (Vector2 v) const
{
    return project(v, params);
}

Params Transformation::d(Vector2 v, int direction) const
{
	static Vector2 stored_v(HUGE_VALF, HUGE_VALF);
    static array<array<Vector2, 2>, 4> result_vectors;
	if (not (exchange(stored_v, v) == v)) {
	    Vector2 projected_v = project(v, params);
	    for (int i=0; i<4; ++i) {
	        Vector3 canonized_v = canonize_matrix[i] * homogenize(v);
	        for (int j=0; j<2; ++j) {
		        result_vectors[i][j] = projection_derivative(projected_v, weight_vector[i].dot(canonized_v), derivative_matrix[i][j] * canonized_v);
			}
	    }
	}
    return pack_vectors(result_vectors, direction);
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

Transformation Transformation::operator () (Transformation other) const
{
    return Transformation(other.region, params * other.params, static_params);
}

float Transformation::scale(Vector2 v) const
{
    Matrix33 m(params);
    Vector3 p = params * homogenize(v);
    set_col(m, 2, p);
    m *= 1./p(2);
    return cv::determinant(m);
}

Vector2 Transformation::operator - (const Transformation &other) const
{
    Vector2 result;
    for (int i=0; i<points.size(); ++i) {
        result += points[i] - static_params[i];
    }
    return (1.0 / points.size()) * result;
}

Transformation Transformation::inverse() const
{
    Region tsf_region = (*this)(region);
    return Transformation(tsf_region, params.inv(), static_params);
}

Vector2 Transformation::inverse(Vector2 v) const
{
    return inverse()(v);
}

std::array<Vector2, 4> Transformation::vertices() const
{
    return static_params;
}
