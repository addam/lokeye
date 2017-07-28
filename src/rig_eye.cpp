#include "main.h"
#include "eye.h"
#include <iostream>

using Annotation = std::tuple<string, string, Circle>;

auto rng = cv::theRNG();

float getfloat(std::stringstream &ss, string delim=",")
{
    static string tmp;
    std::getline(ss, tmp, ',');
    return std::stof(tmp);
}

vector<Annotation> read_csv(std::istream &in)
{
    vector<Annotation> result;
    while (1) {
        string row;
        std::getline(in, row);
        std::stringstream ss;
        ss.str(row);
        string filename;
        std::getline(ss, filename, ',');
        if (filename.empty()) {
            break;
        }
        Circle c;
        c.center[0] = getfloat(ss);
        c.center[1] = getfloat(ss);
        c.radius = getfloat(ss);
        result.emplace_back(row, filename, c);
    }
    return result;
}

Vector2 random(float r)
{
    return {rng.uniform(-r, r), rng.uniform(-r, r)};
}

int main(int argc, char** argv)
{
    auto s1 = new SerialEye;
    s1->add(FindEyePtr(new HoughEye));
    s1->add(FindEyePtr(new LimbusEye));
    auto s2 = new SerialEye;
    auto p2 = new ParallelEye;
    p2->add(FindEyePtr(new HoughEye));
    p2->add(FindEyePtr(new CorrelationEye));
    p2->add(FindEyePtr(new RadialEye(false)));
    s2->add(FindEyePtr(p2));
    s2->add(FindEyePtr(new LimbusEye));
    vector<std::tuple<string, FindEye*>> algorithms = {
        {"hough", new HoughEye},
        {"correlation", new CorrelationEye},
        {"bitmap", new BitmapEye("../data/iris.png", 85 / 100.f)},
        {"limbus", new LimbusEye(10)},
        {"radial", new RadialEye(false)},
        {"hough>limbus", s1},
        {"hough|correlation|radial>limbus", s2},
    };

    std::ios_base::sync_with_stdio(false);
    vector<Annotation> annotations = read_csv(std::cin);
    std::ios_base::sync_with_stdio(true);
    
    string basepath = (argc > 1) ? argv[1] : "";
    using std::get;
    const int repeat = 3;
    Bitmap3 image;
    for (const auto &row : annotations) {
        image.read(basepath + get<1>(row));
        printf("%s\n", get<0>(row).c_str());
        for (const auto &algo : algorithms) {
            for (int i=0; i<repeat; ++i) {
                Circle c = get<2>(row);
                c.center += random(c.radius);
                get<1>(algo)->refit(c, image);
                Vector2 diff = c.center - get<2>(row).center;
                printf(" %s,%.2f,%.2f\n", get<0>(algo).c_str(), diff[0], diff[1]);
            }
        }
    }
    return 0;
}
