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

void match(Face &state, const Bitmap3 &ref, const Bitmap3 &view, bool is_verbose)
{
    TimePoint time_start = std::chrono::high_resolution_clock::now();
    state.render(ref, "reference");
    state.refit(view);
    TimePoint time_now = std::chrono::high_resolution_clock::now();
    float duration = std::chrono::duration_cast<std::chrono::duration<float>>(time_now - time_start).count();
    printf("%g seconds ~= %g fps\n", duration, 1 / duration);
    if (is_verbose) {
        print(state.main_tsf.params);
        std::cout << "Face parameters: " << state() << ", " << 1 / duration << " fps" << std::endl;
    }
    state.render(view, "view");
    cv::waitKey();
}

int main(int argc, char** argv)
{
	bool is_interactive = false, is_verbose = false;
	for (int i=1; i<argc; ++i) {
		string arg(argv[i]);
		if (arg == "-i") {
			is_interactive = true;
		} else if (arg == "-v") {
			is_verbose = true;
        }
    }
    using std::atof;
    Bitmap3 ref, view;
    assert(ref.read(argv[1]));
    assert(view.read(argv[2]));
    try {
        Face state = (is_interactive) ? init_interactive(ref) : init_static(ref);
        match(state, ref, view, is_verbose);
    } catch (NoFaceException) {
        std::cerr << "No face initialized." << std::endl;
        return 1;
    }
    return 0;
}
