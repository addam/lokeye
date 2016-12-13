#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include <iostream>

Matrix35 random_homography()
{
    return Matrix35::randu(-1, 1);
}

vector<Measurement> generate_correspondences(const Matrix35 &h, int count)
{
    vector<Measurement> result;
    Vector4 shift;
    cv::randu(shift, -1e4, 1e4);
    for (int i=0; i<count; ++i) {
        Vector4 x;
        cv::randu(x, -1, 1);
        x[2] *= 100;
        x[3] = x[2];
        x[1] = x[0];
        x += shift;
        Vector2 y;
        cv::randu(y, -1e-3, 1e-3);
        y += dehomogenize(h * homogenize(x));
        result.push_back(std::make_pair(x, y));
    }
    return result;
}

vector<Measurement> generate_random(int count)
{
    vector<Measurement> result;
    Vector4 shift_x;
    cv::randu(shift_x, -1e4, 1e4);
    Vector2 shift_y;
    cv::randu(shift_y, -1e4, 1e4);
    for (int i=0; i<count; ++i) {
        Vector4 x;
        cv::randu(x, -100, 100);
        Vector2 y;
        cv::randu(y, -100, 100);
        result.push_back(std::make_pair(x + shift_x, y + shift_y));
    }
    return result;
}

int main(int argc, char** argv)
{
    Matrix35 h = random_homography();
    h *= 1./cv::norm(h, cv::NORM_L1);
    std::cout << h << std::endl;
    for (int i=17; i < 20; ++i) {
        vector<Measurement> sample = generate_correspondences(h, i);
        TimePoint time_start = std::chrono::high_resolution_clock::now();
        int support = 7;
        Gaze fit(sample, support, 1);
        float duration = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - time_start).count();
        fit.fn *= 1./cv::norm(fit.fn, cv::NORM_L1);
        std::cout << fit.fn << std::endl;
        for (Measurement m : sample) {
            std::cout << m.first << " -> " << project(m.first, h) << " vs. " << fit(m.first) << std::endl;
        }
        //std::cout << "support " << support << ", " << duration << " seconds" << std::endl;
    }
    for (int i=5; i<10; ++i) {
        auto sample = generate_random((i == 9) ? 20 : i);
        int support = 5;
        Gaze fit(sample, support, 0.1);
        for (Measurement m : sample) {
            std::cout << m.first << " -> " << m.second << " vs. " << fit(m.first) << std::endl;
        }
    }
    return 0;
}