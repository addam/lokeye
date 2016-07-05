#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include <iostream>

vector<Measurement> grid_calibrate(Face &face, VideoCapture &cap, Pixel window_size)
{
    const int divisions_x = 16, divisions_y = 9;
    const char winname[] = "calibrate";
    const Vector3 bgcolor(0.7, 0.6, 0.5);
    Bitmap3 canvas(window_size.y, window_size.x);
    canvas = bgcolor;
    cv::namedWindow(winname);
    cv::imshow(winname, canvas);
    Vector2 cell(window_size.x / divisions_x, window_size.y / divisions_y);
    std::random_device rd;
    std::mt19937 generator(rd());
    vector<Vector2> points;
    for (int repeat=0; repeat < 2; ++repeat) {
        int begin = points.size();
        for (int i=0; i<divisions_y; i++) {
            for (int j=0; j<divisions_x; j++) {
                points.emplace_back(Vector2(j * cell[0], i * cell[1]));
            }
        }
        std::shuffle(points.begin() + begin, points.end(), generator);
    }
    vector<Bitmap3> images;
    for (Vector2 point : points) {
        canvas = bgcolor;
        cv::circle(canvas, to_pixel(point), 50, cv::Scalar(0, 0.6, 1), -1);
        cv::circle(canvas, to_pixel(point), 5, cv::Scalar(0, 0, 0.3), -1);
        cv::imshow(winname, canvas);
        cv::waitKey();
        Bitmap3 img;
        img.read(cap);
        images.push_back(img);
        face.record_appearance(img, images.size() == points.size());
    }
    assert(images.size() == points.size());
    vector<Measurement> result;
    for (int i=0; i < images.size(); ++i) {
        const Bitmap3 &img = images[i];
        face.refit(img);
        face.render(img);
        result.emplace_back(std::make_pair(face(img), points[i]));
    }
    return result;
}

void write(const vector<Measurement> &data, std::ostream &ostr)
{
    for (Measurement m : data) {
        ostr << m.first[0] << ' ' << m.first[1] << std::endl;
        ostr << m.second[0] << ' ' << m.second[1] << std::endl;
        ostr << m.first[2] << ' ' << m.first[3] << std::endl;
    }
}

int main(int argc, char** argv)
{
    VideoCapture cam{0};
    Bitmap3 image;
    assert (image.read(cam));
    Face state = mark_eyes(image);
    auto measurements = grid_calibrate(state, cam, Pixel(1600, 900));
    write(measurements, std::cout);
    return 0;
}
