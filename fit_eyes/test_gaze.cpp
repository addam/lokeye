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
        Vector2 y;
        cv::randu(y, -.1, .1);
        y += dehomogenize(h * homogenize(x));
        result.push_back(std::make_pair(x, y));
    }
    return result;
}

int main(int argc, char** argv)
{
    Matrix35 h = random_homography();
    std::cout << h << std::endl;
    for (int i=7; i < 20; ++i) {
        vector<Measurement> sample = generate_correspondences(h, i);
        TimePoint time_start = std::chrono::high_resolution_clock::now();
        int support = 7;
        Gaze fit(sample, support, 1);
        float duration = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - time_start).count();
        std::cout << fit.fn << std::endl;
        for (Measurement m : sample) {
            std::cout << m.first << " -> " << project(m.first, h) << " vs. " << fit(m.first) << std::endl;
        }
        //std::cout << "support " << support << ", " << duration << " seconds" << std::endl;
    }
    return 0;
}
