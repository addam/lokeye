#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include <iostream>

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

Matrix35 random_homography()
{
    return Matrix35::randu(-1, 1);
}

vector<Measurement> generate_correspondences(const Matrix35 &h, int count)
{
    vector<Measurement> result;
    for (int i=0; i<count; ++i) {
        Vector4 x;
        cv::randu(x, -1, 1);
        Vector2 y = dehomogenize(h * homogenize(x));
        result.push_back(std::make_pair(x, y));
    }
    return result;
}

int main(int argc, char** argv)
{
    Matrix35 h = random_homography();
    std::cout << h << std::endl;
    for (int i=10; i < 50; ++i) {
        vector<Measurement> sample = generate_correspondences(h, i);
        TimePoint time_start = std::chrono::high_resolution_clock::now();
        int support = 7;
        Gaze fit(sample, support, 1);
        float duration = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - time_start).count();
        std::cout << fit.fn << std::endl;
        for (Measurement m : sample) {
            std::cout << m.first << " -> " << m.second << " vs. " << fit(m.first) << std::endl;
        }
        std::cout << "support " << support << ", " << duration << " seconds" << std::endl;
    }
    return 0;
}
