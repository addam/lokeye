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

class Eigenface
{
    const static int count = 2;
    Region region;
    Matrix data;
    Matrix subspace;
    Matrix remap(const Bitmap3 &image, const Transformation &tsf) const;
public:
    Eigenface(const Face&);
    void add(const Bitmap3&, Transformation tsf=Transformation());
    cv::Vec<float, count> evaluate(const Bitmap3&, Transformation) const;
};

Matrix Eigenface::remap(const Bitmap3 &image, const Transformation &tsf) const
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

Eigenface::Eigenface(const Face &face): region(face.region)
{
}

void Eigenface::add(const Bitmap3 &image, Transformation tsf)
{
    data.push_back(remap(image, tsf));
    subspace = cv::PCA(data, cv::noArray(), cv::PCA::DATA_AS_ROW, count).eigenvectors;
}

cv::Vec<float, Eigenface::count> Eigenface::evaluate(const Bitmap3 &image, Transformation tsf) const
{
    cv::Vec<float, count> result;
    Matrix warp = remap(image, tsf);
    result = Matrix(subspace * warp.t());
    return result;
}

int main(int argc, char** argv)
{
    VideoCapture cam{0};
    Bitmap3 image;
    assert (image.read(cam));
    Face state = mark_eyes(image);
    std::cout << state.region << std::endl;
    Eigenface appearance(state);
    appearance.add(image, state.tsf);
    bool do_add = true;
    while (char(cv::waitKey(5)) != 27 and image.read(cam)) {
        if (do_add) {
            TimePoint time_start = std::chrono::high_resolution_clock::now();
            state.refit(image);
            appearance.add(image, state.tsf);
            float duration = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - time_start).count();
            printf(" adding a face took %g s\n", duration);
        } else {
            TimePoint time_start = std::chrono::high_resolution_clock::now();
            auto projection = appearance.evaluate(image, state.tsf);
            float duration = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - time_start).count();
            printf(" [%g, %g]; processing took %g s, \n", projection[0], projection[1], duration);
        }
        char c = cv::waitKey(1);
        if (c == 27) {
            break;
        } else if (c == 'a') {
            do_add ^= true;
        }
    }
    return 0;
}
