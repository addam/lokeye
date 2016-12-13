#include "vector_math.h"
#include <iostream>

namespace
{

template<int N>
inline cv::Matx<float, N, N> cross_axes(int i, int j)
{
    cv::Matx<float, N, N> result = cv::Matx<float, N, N>::zeros();
    result(i, j) = 1;
    result(j, i) = -1;
    return result;
}

}

template<int N, int M>
float evaluate_homography(const cv::Matx<float, N, M> &h, const vector<std::pair<cv::Vec<float, M-1>, cv::Vec<float, N-1>>> &pairs)
{
    float value = 0;
    for (auto pair : pairs) {
        cv::Vec<float, N-1> p = project(pair.first, h);
        value += cv::norm(p - pair.second, cv::NORM_L2SQR);
    }
    return 0.5 * value;
}

template<int N, int M>
void refit_homography(cv::Matx<float, N, M> &h, const vector<std::pair<cv::Vec<float, M-1>, cv::Vec<float, N-1>>> &pairs)
{
    using Homography = cv::Matx<float, N, M>;
    for (int iteration=0; iteration<10; iteration++) {
        Homography gradient = Homography::zeros();
        for (auto pair : pairs) {
            cv::Vec<float, N-1> projection = project(pair.first, h);
            cv::Vec<float, N-1> diff = projection - pair.second;
            cv::Vec<float, N> rowwise_component;
            for (int i=0; i<N-1; i++) {
                rowwise_component[i] = diff[i];
                rowwise_component[N-1] -= diff[i] * projection[i];
            }
            gradient += Homography(Matrix(Matrix(rowwise_component) * Matrix(homogenize(pair.first)).t()));
        }
        float value = evaluate_homography(h, pairs);
        float grad_norm = cv::norm(gradient, cv::NORM_L2SQR);
        float step_size = 1 / grad_norm;//value / std::max(1.f, grad_norm);
        while (evaluate_homography(h - gradient * step_size, pairs) > value) {
            step_size /= 2;
            if (step_size < 1e-20) {
                //printf("resulting value: %g; stop because step_size = %g\n", evaluate_homography(h, pairs), step_size);
                return;
            }
        }
        //printf("value after %i iterations: %g; choosing step %g\n", i, value, step_size);
        //std::cout << gradient * step_size << std::endl;
        h -= gradient * step_size;
    }
    //printf("resulting value: %g\n", evaluate_homography(h, pairs));
}

/** Direct linear transform from pair.first to pair.second
 */
template<int N, int M>
cv::Matx<float, N, M> homography(const vector<std::pair<cv::Vec<float, M-1>, cv::Vec<float, N-1>>> &pairs)
{
    using Homography = cv::Matx<float, N, M>;
    cv::Vec<float, M-1> center_left, variance_left;
    cv::Vec<float, N-1> center_right, variance_right;
    int count = 0;
    for (auto pair : pairs) {
        count += 1;
        variance_left += pow2(pair.first - center_left) * (count - 1) / count;
        variance_right += pow2(pair.second - center_right) * (count - 1) / count;
        center_left += (pair.first - center_left) / count;
        center_right += (pair.second - center_right) / count;
    }
    cv::Vec<float, M-1> stddev_left = sqrt(variance_left);
    cv::Vec<float, N-1> stddev_right = sqrt(variance_right);
    Matrix system(0, N*M);
    for (auto pair : pairs) {
        auto lhs = homogenize((pair.first - center_left) / stddev_left);
        auto rhs = homogenize((pair.second - center_right) / stddev_right);
        int i = max_component(rhs);
        for (int j=0; j<N; ++j) {
            if (i != j) {
                Matrix row(cross_axes<N>(i, j) * rhs * lhs.t());
                system.push_back(row.reshape(1, 1));
            }
        }
    }
    Matrix vt = cv::SVD(system, cv::SVD::FULL_UV).vt;
    float best_error = 1e20;
    Homography result;
    Matrix normalize = scaling(stddev_left, true) * translation(center_left, true);
    Matrix denormalize = translation(center_right) * scaling(stddev_right);
    Matrix tmp_best_h;
    for (int i=0; i<vt.rows; ++i) {
        Matrix h = vt.row(i).reshape(1, N);
        Homography candidate = Matrix(denormalize * h * normalize);
        float error = evaluate_homography<N, M>(candidate, pairs);
        if (error < best_error) {
            best_error = error;
            result = candidate;
            tmp_best_h = h;
        }
    }
    static size_t prev_count = 0;
    if (false and prev_count != pairs.size()) {
        std::cout << "solving:\nin = [";
        for (auto pair:pairs) {
            std::cout << homogenize(pair.first) /*homogenize((pair.first - center_left) / stddev_left)*/ << ';' << std::endl;
        }
        std::cout << "]';\nout = [";
        for (auto pair:pairs) {
            std::cout << homogenize(pair.second) /*homogenize((pair.second - center_right) / stddev_right)*/ << ';' << std::endl;
        }
        std::cout << "]';\n";
        std::cout << "system = " << system << ';' << std::endl;
        auto svd = cv::SVD(system, cv::SVD::FULL_UV);
        std::cout << "u:\n" /*<< svd.u*/ << "\nd:\n" << svd.w << "\nvt:\n" << svd.vt << std::endl;
        std::cout << "result\n" << denormalize << " * " << tmp_best_h << " * " << normalize << std::endl;
        prev_count = pairs.size();
    }
    if (pairs.size() > (M*N - 1) / (N-1)) {
        //printf("initial %g, ", evaluate_homography(result, pairs));
        refit_homography(result, pairs);
        //printf("final %g\n", evaluate_homography(result, pairs));
    }
    return result;
}
