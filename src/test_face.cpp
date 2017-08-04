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

std::basic_ostream<char> &operator<<(std::basic_ostream<char> &stream, const Triangle &t)
{
    return (stream << "[a = " << t[0] << ", b = " << t[1] << ", c = " << t[2] << "]");
}
    
void track_interactive(VideoCapture &cam, bool is_interactive, bool is_verbose)
{
    Bitmap3 image;
    for (int i=0; i<10; i++) {
        assert (image.read(cam));
    }
    Face state = (is_interactive) ? init_interactive(image) : init_static(image);
    if (1) {
        auto serial = new SerialEye;
        serial->add(FindEyePtr(new HoughEye(Vector3(0.7, 0, 0))));
        serial->add(FindEyePtr(new LimbusEye));
        state.eye_locator.reset(serial);
    } else if (0) {
        state.eye_locator.reset(new CorrelationEye);
    } else if (1) {
        state.eye_locator.reset(new RadialEye(false));
    } else {
        // iris diameter is 85 / 100 of the size of this image
        state.eye_locator.reset(new BitmapEye("../data/iris.png", 85 / 100.f));
    }
    
    std::cout << state.main_tsf.region << std::endl;
    TimePoint time_start = std::chrono::high_resolution_clock::now();
    TimePoint time_prev = time_start;
    int i;
    for (i=0; char(cv::waitKey(1)) != 27 and image.read(cam); ++i) {
        state.refit(image);
        if (is_verbose) {
            TimePoint time_now = std::chrono::high_resolution_clock::now();
            float duration = std::chrono::duration_cast<std::chrono::duration<float>>(time_now - time_prev).count();
            //std::cout << "Main transformation: " << state.main_tsf.params << ", face parameters: " << state() << ", " << 1 / duration << " fps" << std::endl;
            print(state.main_tsf.params);
            std::cout << "Face parameters: " << state() << ", " << 1 / duration << " fps" << std::endl;
            time_prev = time_now;
        }
        state.render(image, "tracking");
    }
    float duration = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - time_start).count();
    printf("%i frames in %g seconds ~= %g fps\n", i, duration, i / duration);
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
    VideoCapture cam{0};
    try {
        track_interactive(cam, is_interactive, is_verbose);
    } catch (NoFaceException) {
        std::cerr << "No face initialized." << std::endl;
        return 1;
    }
    return 0;
}
