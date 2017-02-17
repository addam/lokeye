#include "main.h"
#include "bitmap.h"
#include "optimization.h"
#include <memory>
#include <iostream>
const char winname[] = "image";
Region region;
std::unique_ptr<Transformation> tsf, approx;
Bitmap3 view, ref;

void refit_transformation(Transformation&, Region, const Bitmap3 &img, const Bitmap3 &ref, int min_size);

template<size_t N>
void render_polygon(Bitmap3 &canvas, const std::array<Vector2, N> vertices, float r, float g, float b)
{
    for (size_t i = 0; i < N; i++) {
        cv::line(canvas, to_pixel(vertices[i]), to_pixel(vertices[(i+1)%N]), cv::Scalar(b, g, r));
    }	
}

void render()
{
	Bitmap3 canvas = view.clone();
    std::array<Vector2, 3> vertices = {vectorize(tsf->points.col(0)), vectorize(tsf->points.col(1)), vectorize(tsf->points.col(2))};
    render_polygon(canvas, vertices, 0, 1, 0);
    std::for_each(vertices.begin(), vertices.end(), [](Vector2 &v) { v = (*approx)(v); });
    render_polygon(canvas, vertices, 1, 1, 0);
	cv::imshow(winname, canvas);
}

float sqr_distance(float x, float y, cv::Matx<float, 2, 1> point)
{
	return pow2(x - point(0)) + pow2(y - point(1));
}

#ifdef TRANSFORMATION_BARYCENTRIC_H
void onmouse3(int event, int x, int y, int, void* param)
{
	static int selected = -1;
    if (event == cv::EVENT_LBUTTONDOWN) {
		Matrix23 points = tsf->points;
	    selected = 0;
	    for (int i=1; i<3; i++) {
			if (sqr_distance(x, y, points.col(i)) < sqr_distance(x, y, points.col(selected))) {
				selected = i;
			}
		}
	} else if (selected >= 0 and event == cv::EVENT_MOUSEMOVE) {
	    Matrix23 delta = Matrix23::zeros();
	    delta.col(selected) = Vector2(x, y) - tsf->points.col(selected);
	    for (int i=0; i<2; ++i) {
			delta(i, selected) = ((i == 0) ? x : y) - tsf->points(i, selected);
		}
	    *tsf += delta;
	} else if (event == cv::EVENT_LBUTTONUP) {
		selected = -1;
	} else {
		return;
	}
	Transformation tsf_inv = tsf->inverse();
	for (Pixel p : view) {
		view(p) = ref(tsf_inv(p));
	}
	render();
}
#endif

void onmouse2(int event, int x, int y, int, void* param)
{
    if (event == cv::EVENT_MOUSEMOVE) {
	    region = Region(Pixel(region.x, region.y), Pixel(x, y));
	} else if (event == cv::EVENT_LBUTTONUP) {
		printf("up. Region: %g, %g ... %g, %g\n", region.x, region.y, region.x + region.width, region.y + region.height);
		cv::setMouseCallback(winname, onmouse3);
		tsf.reset(new Transformation(region));
		approx.reset(new Transformation(region));
	}
}

void onmouse1(int event, int x, int y, int, void* param)
{
    if (event == cv::EVENT_LBUTTONDOWN) {
	    region = Region(x, y, 1, 1);
		cv::setMouseCallback(winname, onmouse2);
	}
}

int main()
{
    cv::VideoCapture cam(0);
	ref.read(cam);
	view = ref.clone();
	cv::namedWindow(winname);
	cv::setMouseCallback(winname, onmouse1);
	cv::imshow(winname, ref);
	if (char(cv::waitKey()) == 27) {
		return 1;
	}
	printf("fitting\n");
	while (char(cv::waitKey(10)) != 27) {
		refit_transformation(*approx, region, view, ref, 3);
		render();
		std::cout << "params: " << approx->params << std::endl;
	}
}
