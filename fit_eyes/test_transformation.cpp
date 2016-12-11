#include "main.h"
#include "optimization.h"
#include <iostream>
#include <random>

using Params = Transformation::Params;
const int param_cn = Params::rows * Params::cols;
using Gradient = cv::Matx<float, 2, param_cn>;
using Rng = std::uniform_real_distribution<>;

template<int N, int M>
void param_set(cv::Matx<float, N, M> &params, int i, float value)
{
    params(i/M, i%M) = value;
}

template<int N>
void param_set(cv::Vec<float, N> &params, int i, float value)
{
    params(i) = value;
}

template<int N, int M>
float param_get(const cv::Matx<float, N, M>  &params, int i)
{
    return params(i/Params::cols, i%Params::cols);
}

template<int N>
float param_get(const  cv::Vec<float, N> &params, int i)
{
    return params(i);
}

std::basic_ostream<char> &operator<<(std::basic_ostream<char> &stream, const std::array<cv::Vec<float, 2>, 4> arr)
{
    using std::endl;
    return (stream << arr[0] << endl << arr[1] << endl << arr[2] << endl << arr[3]);
}
    

Params rand_params(std::minstd_rand &gen)
{
    Params result;
    for (int i=0; i<param_cn; ++i) {
        param_set(result, i, Rng(-1, 1)(gen));
    }
    return result;
}

cv::Point2f rand_point(std::minstd_rand &gen)
{
    Vector2 result;
    result[0] = Rng(0, 640)(gen);
    result[1] = Rng(0, 480)(gen);
    return result;
}

Gradient analytic(const Transformation &tsf, Vector2 point)
{
    Gradient result;
    for (int i=0; i<2; ++i) {
        const Params row = tsf.d(point, i);
        for (int j=0; j<param_cn; ++j) {
            result(i, j) = param_get(row, j);
        }
    }
    return result;
}

Gradient numeric(const Transformation &tsf, Vector2 point, float delta)
{
    Gradient result;
    Vector2 tsf_point = tsf(point);
    for (int j=0; j<param_cn; ++j) {
        Params elem;
        for (int t=0; t<param_cn; ++t) {
            param_set(elem, t, (j == t) ? delta : 0);
        }
        Vector2 col = (tsf + elem)(point) - tsf_point;
        for (int i=0; i<2; ++i) {
            result(i, j) = col(i) / delta;
        }
    }
    return result;
}

int main(int argc, char** argv)
{
    using std::cout;
    using std::endl;
    std::random_device rd;
    std::minstd_rand gen(rd());
    Region region(rand_point(gen), rand_point(gen));
    cout << region << endl;
    Transformation tsf(region);
    Vector2 point = rand_point(gen);
    Params r = rand_params(gen);
    cout << "Static:" << endl << tsf.static_params << endl;
    cout << tsf.params << endl << r << endl;
    #if 0
        tsf += r;
        cout << tsf.params << endl;
    #endif
    cout << "Analytic:" << endl << analytic(tsf, point) << endl;
    float delta = 1;
    for (int i=0; i<2; ++i) {
        cout << "Numeric, delta " << delta << ":" << endl << numeric(tsf, point, delta) << endl;
        delta /= 10;
    }
    return 0;
}
