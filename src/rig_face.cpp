#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include <iostream>

// locrot tracker
void print(const std::pair<Vector2, float> &params)
{
    std::cout << "Main transformation shift: " << params.first << ", angle: " << params.second << std::endl;
}    

// affine tracker
void print(const std::pair<Vector2, Matrix22> &params)
{
    std::cout << "Main transformation shift: " << params.first << ", linear: " << params.second << std::endl;
}    

// barycentric tracker
void print(const Matrix23 &params)
{
    std::cout << "Main transformation: " << params << std::endl;
}

// perspective tracker
void print(const Matrix33 &params)
{
    std::cout << "Main transformation: " << params << std::endl;
}

int main(int argc, char** argv)
{
    using std::atof;
    Bitmap3 ref, view;
    assert(ref.read(argv[1]));
    assert(view.read(argv[2]));
    Region r;
    if (argc >= 7) {
        r.x = atof(argv[3]);
        r.y = atof(argv[4]);
        r.width = atof(argv[5]);
        r.height = atof(argv[6]);
    }
    Face state = (r.x > 0) ? init_static(ref, r) : init_interactive(ref);
    if (argc < 7) {
        Region r = state.main_region;
        std::cout << r.x << ' ' << r.y << ' ' << r.width << ' ' << r.height << std::endl;
    }
    TimePoint time_start = std::chrono::high_resolution_clock::now();
    state.refit(view);
    TimePoint time_now = std::chrono::high_resolution_clock::now();
    float duration = std::chrono::duration_cast<std::chrono::duration<float>>(time_now - time_start).count();
    printf("%g seconds ~= %g fps\n", duration, 1 / duration);
    print(state.main_tsf.params);
    std::cout << "Face parameters: " << state() << ", " << 1 / duration << " fps" << std::endl;
    state.render(view, "tracking");
    cv::waitKey();
    return 0;
}
