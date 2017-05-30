#ifndef SUBPIXEL_HPP
#define SUBPIXEL_HPP
#include <opencv2/core.hpp>

template<typename InputArray>
void find_maximum(InputArray score, float &x, float &y)
{
	using Mat = cv::Mat_<float>;
	cv::Point origin;
	cv::minMaxLoc(score, 0, 0, 0, &origin);
	origin.x = std::max(1, std::min(origin.x, score.cols - 2));
	origin.y = std::max(1, std::min(origin.y, score.rows - 2));
	Mat system, rhs;
	for (int x=-1; x<=1; ++x) {
		for (int y=-1; y<=1; ++y) {
			Mat tmp(1, 6);
			tmp(0) = x*x;
			tmp(1) = x*y;
			tmp(2) = x;
			tmp(3) = y*y;
			tmp(4) = y;
			tmp(5) = 1;
			system.push_back(tmp);
			rhs.push_back(score.template at<float>(cv::Point(x, y) + origin));
		}
	}
	Mat v;
	cv::solve(system, rhs, v, cv::DECOMP_SVD);
	Mat polynomial(3, 3);
	polynomial << 2*v(0), v(1), v(2), v(1), 2*v(3), v(4), v(2), v(4), 2*v(5);
	Mat result;
	cv::solve(polynomial.colRange(0, 2).rowRange(0, 2), polynomial.rowRange(0, 2).col(2), result);
	x = origin.x - result(0);
	y = origin.y - result(1);
}

#if 0
int main()
{
	cv::Mat_<float> score(100, 100);
	auto rng = cv::theRNG();
	for (int i=0; i<10; ++i) {
		float x0 = rng.uniform(10.f, 90.f), y0 = rng.uniform(10.f, 90.f);
		float a = rng.uniform(-10.f, -0.1f), b = rng.uniform(-10.f, -0.1f), c = rng.uniform(2*a, -2*a), d = rng.uniform(-1e5f, 1e5f);
		for (int i=0; i<score.rows; ++i) {
			for (int j=0; j<score.rows; ++j) {
				float x = j - x0, y = i - y0;
				score(i, j) = a*x*x + b*y*y + c*x*y + d + rng.uniform(a, -a);
			}
		}
		float x, y;
		find_maximum(score, x, y);
		printf("true maximum: %g, %g; calculated: %g, %g\n", x0, y0, x, y);
	}
}
#endif

#endif
