#include "main.h"
#include "optimization.h"
#include "homography.h"
#include "transformation_perspective.h"
// TEMPORARY DEBUG
#include <iostream>
namespace {
	std::basic_ostream<char> &operator<<(std::basic_ostream<char> &stream, const std::array<cv::Vec<float, 2>, 4> arr)
{
    using std::endl;
    return (stream << arr[0] << endl << arr[1] << endl << arr[2] << endl << arr[3]);
}
}
    


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

Matrix33 decanonize(PointPack points)
{
    Matrix33 system;
    for (int i=0; i<3; ++i) {
        set_col(system, i, homogenize(points[i]));
    }
    Vector3 alpha = system.solve(homogenize(points[3]), cv::DECOMP_LU);
    for (int i=0; i<3; ++i) {
        set_col(system, i, alpha(i) * homogenize(points[i]));
    }
    return system;
}

Vector2 extract_point(Params p, unsigned index)
{
    assert (index < 4);
    return Vector2(p[2*index + 0], p[2*index + 1]);
}

//vector<Measurement> extract_measurements(Params p, const decltype(Transformation::static_params) &s)
//{
    //vector<Measurement> result;
    //for (int i=0; i<4; ++i) {
        //result.emplace_back(std::make_pair(extract_point(p, i), s[i]));
    //}
    //return result;
//}

vector<std::pair<Vector2, Vector2>> zip_measurements(const PointPack &left, const PointPack &right)
{
	vector<std::pair<Vector2, Vector2>> result;
	for (int i=0; i<4; ++i) {
		result.emplace_back(std::make_pair(left[i], right[i]));
	}
	return result;
}

Params pack_vectors(const PointPack &s)
{
    Params result;
    for (int i=0; i<4; ++i) {
        result[2*i + 0] = s[i][0];
        result[2*i + 1] = s[i][1];
    }
    return result;
}

Matrix23 projection_derivative(Vector2 projected_v, float w)
{
    return (1./w) * Matrix23{1, 0, -projected_v[0],
        0, 1, -projected_v[1]};
}

template<typename T>
void cycle(T &seq)
{
    std::rotate(seq.begin(), seq.begin() + 1, seq.end());
}

decltype(Transformation::canonize_matrix) canonize_all_permutations(PointPack points)
{
    array<Matrix33, 4> result{};
    for (int i=0; i<4; ++i) {
        result[i] = canonize(points);
        cycle(points);
    }
    return result;
}

}

Transformation::Transformation():
    static_params{Vector2{0, 0}, Vector2{0, 1}, Vector2{1, 1}, Vector2{1, 0}},
    canonize_matrix(canonize_all_permutations(static_params))
{
    points = static_params;
    update_params(Matrix33::eye());
}

Transformation::Transformation(Region region):
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
    //std::cout << "Update points:\n" << points << std::endl << "params:\n" << params << std::endl;
    for (int i=0; i<4; ++i) {
		PointPack points_replaced_dx = points, points_replaced_dy = points;
		points_replaced_dx.back() = Vector2{1, 0};
		points_replaced_dy.back() = Vector2{0, 1};
        derivative_matrix[0][i] = decanonize(points_replaced_dx);
        derivative_matrix[1][i] = decanonize(points_replaced_dy);
        weight_vector[i] = vectorize(decanonize(points).row(2));
        //std::cout << "Update " << i << " points_replaced:\n" << points_replaced_dx << std::endl << points_replaced_dy << std::endl << derivative_matrix[0][i] << std::endl << derivative_matrix[1][i] << std::endl << "weight " << weight_vector[i] << std::endl;
        cycle(points);
    }
}

Transformation::Transformation(decltype(params) in_params, decltype(static_params) static_params):
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

Transformation Transformation::operator + (Params delta) const
{
    Transformation result(*this);
    result += delta;
    return result;
}

Transformation& Transformation::operator += (Params delta)
{
    for (int i=0; i<points.size(); ++i) {
        points[i] += extract_point(delta, i);
    }
    update_params(homography<3, 3>(zip_measurements(static_params, points)));
    return *this;
}

Vector2 Transformation::operator () (Vector2 v) const
{
    return project(v, params);
}

Params Transformation::d(Vector2 v, int direction) const
{
    array<Vector2, 4> result_vectors;
    Vector2 projected_v = project(v, params);
    for (int i=0; i<4; ++i) {
        Vector3 canonized_v = canonize_matrix[i] * homogenize(v);
        result_vectors[i] = projection_derivative(projected_v, weight_vector[i].dot(canonized_v)) * derivative_matrix[direction][i] * canonized_v;
        std::cout << "point " << i << ":  ------------\n" << projection_derivative(projected_v, weight_vector[i].dot(canonized_v)) << " *\n" << derivative_matrix[direction][i] << " *\n" << canonize_matrix[i] << " * " << homogenize(v) << std::endl;
    }
    return pack_vectors(result_vectors);
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
    return Transformation(params * other.params, static_params);
}

float Transformation::scale(Vector2 v) const
{
    return (params(0, 0) * params(1, 1)) / (params.row(2) * homogenize(v))[0];
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
    return Transformation(params.inv(), static_params);
}

Vector2 Transformation::inverse(Vector2 v) const
{
    return inverse()(v);
}
