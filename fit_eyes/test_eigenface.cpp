#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include <iostream>

Color pow2(Color x)
{
    return Color(pow2(x[0]), pow2(x[1]), pow2(x[2]));
}

Color sqrt(Color x)
{
    return Color(std::sqrt(x[0]), std::sqrt(x[1]), std::sqrt(x[2]));
}

Color operator/ (Color a, Color b)
{
    return Color(a[0] / b[0], a[1] / b[1], a[2] / b[2]);
}

Color operator* (Color a, Color b)
{
    return Color(a[0] * b[0], a[1] * b[1], a[2] * b[2]);
}

Matrix remap(const Bitmap3 &image, Region region, const Transformation &tsf=Transformation())
{
    Bitmap3 warp(to_rect(region));
    int count = 0;
    Color mean(0, 0, 0), variance(0, 0, 0);
    for (Pixel p : warp) {
        Color value = warp(p) = image(tsf(warp.to_world(p)));
        count += 1;
        variance += pow2(mean - value) * (count - 1) / count;
        mean += (value - mean) / count;
    }
    Color stddev = sqrt(variance / count);
    for (Pixel p : warp) {
        warp(p) = (warp(p) - mean) / stddev;
    }
    Matrix result = warp.reshape(1, 1);
    return result;
}

Bitmap3 reshape(const Matrix &data, Region region)
{
    Bitmap3 result = ColorMatrix(data.reshape(3, to_rect(region).height));
    float h = 1e10;
    Color min(h, h, h), max(-h, -h, -h);
    for (Pixel p : result) {
        Color value = result(p);
        for (int i=0; i<3; i++) {
            min[i] = std::min(min[i], value[i]);
            max[i] = std::max(max[i], value[i]);
        }
    }
    for (Pixel p : result) {
        result(p) = min + result(p) / (max - min);
    }    
    return result;
}

Matrix eigensamples(Matrix samples, int count)
{
    cv::PCA subspace(samples, cv::noArray(), cv::PCA::DATA_AS_ROW, count);
    return subspace.eigenvectors;
}

int main(int argc, char** argv)
{
    VideoCapture cam{0};
    Bitmap3 image;
    assert (image.read(cam));
    Face state = mark_eyes(image);
    std::cout << state.region << std::endl;
    Matrix data = remap(image, state.region, state.tsf);
    for (int i=0; char(cv::waitKey(5)) != 27 and image.read(cam); i++) {
        TimePoint time_start = std::chrono::high_resolution_clock::now();
        state.refit(image);
        data.push_back(remap(image, state.region, state.tsf));
        Matrix eigenfaces = eigensamples(data, 2);
        float duration = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - time_start).count();
        printf(" processing %i faces took %g s (%g s / face)\n", data.rows, duration, duration / data.rows);
        cv::imshow("first", reshape(eigenfaces.row(0), state.region));
        cv::imshow("second", reshape(eigenfaces.row(1), state.region));
        cv::waitKey(1);
        std::cout << state.tsf.params << std::endl;
        state.render(image);
    }
    return 0;
}
