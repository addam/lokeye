#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H
#include <opencv2/core.hpp>

using Vector2 = cv::Vec2f;
using Vector3 = cv::Vec3f;
using Vector4 = cv::Vec<float, 4>;
using Vector5 = cv::Vec<float, 5>;
using Vector6 = cv::Vec<float, 6>;
using Vector8 = cv::Vec<float, 8>;
using Matrix21 = cv::Matx<float, 2, 1>;
using Matrix22 = cv::Matx22f;
using Matrix23 = cv::Matx23f;
using Matrix32 = cv::Matx32f;
using Matrix33 = cv::Matx33f;
using Matrix35 = cv::Matx<float, 3, 5>;
using Matrix55 = cv::Matx<float, 5, 5>;

using Color = Vector3;
using Matrix = cv::Mat_<float>;
using ColorMatrix = cv::Mat_<Color>;

template<int N>
cv::Matx<float, N, 1> colmatrix(cv::Vec<float, N> v)
{
	return static_cast<cv::Matx<float, N, 1>>(v);
}

template<int N>
cv::Matx<float, 1, N> rowmatrix(cv::Vec<float, N> v)
{
	return static_cast<cv::Matx<float, N, 1>>(v).t();
}

template<int N, int M, typename Vector=cv::Matx<float, 1, M>>
void set_row(cv::Matx<float, N, M> &matrix, int i, const Vector &vec)
{
	for (int j=0; j<M; ++j) {
		matrix(i, j) = vec(j);
	}
}

template<int N, int M, typename Vector=cv::Matx<float, N, 1>>
void set_col(cv::Matx<float, N, M> &matrix, int j, const Vector &vec)
{
	for (int i=0; i<N; ++i) {
		matrix(i, j) = vec(i);
	}
}

template<int N, int M, typename Vector=cv::Matx<float, N, 1>>
void add_col(cv::Matx<float, N, M> &matrix, int j, const Vector &vec)
{
	for (int i=0; i<N; ++i) {
		matrix(i, j) += vec(i);
	}
}

template<int N>
cv::Vec<float, N> vectorize(cv::Matx<float, N, 1> m)
{
	cv::Vec<float, N> result;
	result += m;
	return result;
}

template<int N>
cv::Vec<float, N> vectorize(cv::Matx<float, 1, N> m)
{
	return vectorize(m.t());
}

template<int N>
cv::Vec<float, N+1> homogenize (cv::Vec<float, N> v)
{
    cv::Vec<float, N+1> result;
    for (int i=0; i<N; i++) {
        result[i] = v[i];
    }
    result[N] = 1;
    return result;
}

template<int N>
cv::Vec<float, N-1> dehomogenize (cv::Vec<float, N> v)
{
    cv::Vec<float, N-1> result;
    const int last = N - 1;
    for (int i=0; i<last; i++) {
        result[i] = v[i] / v[last];
    }
    return result;
}

template<int N, int M>
cv::Vec<float, N - 1> project(cv::Vec<float, M - 1> v, const cv::Matx<float, N, M> &h)
{
    return dehomogenize(h * homogenize(v));
}

template<int N>
cv::Vec<float, N> operator/ (cv::Vec<float, N> a, cv::Vec<float, N> b)
{
    decltype(a) result = a;
    for (int i=0; i<N; i++) {
        result[i] /= b[i];
    }
    return result;
}

template<int N>
cv::Vec<float, N> pow2(cv::Vec<float, N> v)
{
    decltype(v) result;
    for (int i=0; i<N; i++) {
        result[i] = v[i] * v[i];
    }
    return result;
}

template<int N>
cv::Vec<float, N> sqrt(cv::Vec<float, N> v)
{
    decltype(v) result;
    for (int i=0; i<N; i++) {
        result[i] = std::sqrt(v[i]);
    }
    return result;
}

template<int N>
unsigned max_component (cv::Vec<float, N> v)
{
    unsigned result = 0;
    float max_value = 0;
    for (unsigned i=0; i<N; ++i) {
        if (std::abs(v[i]) > max_value) {
            max_value = std::abs(v[i]);
            result = i;
        }
    }
    return result;
}

template<int N>
Matrix translation(cv::Vec<float, N> v, bool inverse=false)
{
    Matrix result = Matrix::eye(N + 1, N + 1);
    for (int i=0; i<N; i++) {
        result(i, N) = inverse ? -v[i] : v[i];
    }
    return result;
}

template<int N>
Matrix scaling(cv::Vec<float, N> v, bool inverse=false)
{
    Matrix result = Matrix::zeros(N + 1, N + 1);
    for (int i=0; i<N; i++) {
        result(i, i) = inverse ? 1/v[i] : v[i];
    }
    result(N, N) = 1;
    return result;
}
#endif
