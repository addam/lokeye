#include "main.h"
#include "eye.h"
#include <iostream>

using std::string;
using Annotation = std::tuple<string, string, Circle>;

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

Vector2 random(float radius)
{
    return {0, 0}; /// @todo fixme
}

int main(int argc, char** argv)
{
    vector<std::tuple<string, FindEye*>> algorithms = {
        {"hough", new HoughEye},
        {"correlation", new CorrelationEye},
        {"limbus", new LimbusEye},
        {"radial", new RadialEye(false)},
        {"bitmap", new BitmapEye("../data/iris.png", 85 / 100.f)},
    };

    std::ios_base::sync_with_stdio(false);
    vector<Annotation> annotations = read_csv(std::cin);
    std::ios_base::sync_with_stdio(true);
    
    string relpath = (argc > 1) ? argv[1] : "";
    using std::get;
    const int repeat = 1;
    Bitmap3 image;
    for (const auto &row : annotations) {
        //std::clog << "read " << (relpath + get<0>(row)) << std::endl;
        image.read(relpath + get<1>(row));
        printf("%s\n", get<0>(row).c_str());
        for (const auto &algo : algorithms) {
            //std::clog << get<0>(algo) << std::endl;
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