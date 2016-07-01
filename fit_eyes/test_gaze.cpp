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
    for (int i=0; i<count; ++i) {
        Vector4 x;
        cv::randu(x, -1, 1);
        x[2] *= 100;
        x[3] = x[2];
        x[1] = x[0];
        Vector2 y;
        cv::randu(y, -1e-5, 1e-5);
        y += dehomogenize(h * homogenize(x));
        result.push_back(std::make_pair(x, y));
    }
    return result;
}

int main(int argc, char** argv)
{
    Matrix35 h = random_homography();
    h *= 1./cv::norm(h, cv::NORM_L1);
    std::cout << h << std::endl;
    for (int i=197; i < 200; ++i) {
        vector<Measurement> sample = generate_correspondences(h, i);
        TimePoint time_start = std::chrono::high_resolution_clock::now();
        int support = 7;
        Gaze fit(sample, support, 1);
        float duration = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - time_start).count();
        fit.fn *= 1./cv::norm(fit.fn, cv::NORM_L1);
        std::cout << fit.fn << std::endl;
        for (Measurement m : sample) {
            //std::cout << m.first << " -> " << project(m.first, h) << " vs. " << fit(m.first) << std::endl;
        }
        //std::cout << "support " << support << ", " << duration << " seconds" << std::endl;
    }
    return 0;
}
