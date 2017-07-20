#include "eye.h"

Circle Circle::average(const vector<Circle> &seq)
{
    Circle result = {{0, 0}, 0};
    for (const Circle &c : seq) {
        result.center += c.center;
        result.radius += c.radius;
    }
    result.center *= 1./seq.size();
    result.radius *= 1./seq.size();
    return result;
}

/** Get quadratic-interpolated maximum location in world coordinates
 */
Vector2 precise_maximum(Bitmap1 score)
{
	cv::Point origin;
	cv::minMaxLoc(score, 0, 0, 0, &origin);
	origin.x = std::max(1, std::min(origin.x, score.cols - 2));
	origin.y = std::max(1, std::min(origin.y, score.rows - 2));
	Matrix system, rhs;
	for (int x=-1; x<=1; ++x) {
		for (int y=-1; y<=1; ++y) {
			Matrix tmp(1, 6);
			tmp(0) = x*x;
			tmp(1) = x*y;
			tmp(2) = x;
			tmp(3) = y*y;
			tmp(4) = y;
			tmp(5) = 1;
			system.push_back(tmp);
			rhs.push_back(score.at<float>(cv::Point(x, y) + origin));
		}
	}
	Matrix v;
	cv::solve(system, rhs, v, cv::DECOMP_SVD);
	Matrix polynomial(3, 3);
	polynomial << 2*v(0), v(1), v(2), v(1), 2*v(3), v(4), v(2), v(4), 2*v(5);
	Matrix result;
	cv::solve(polynomial.colRange(0, 2).rowRange(0, 2), polynomial.rowRange(0, 2).col(2), result);
	return score.to_world(origin) - Vector2{result(0), result(1)};
}

void ParallelEye::add(FindEyePtr&& child)
{
    children.emplace_back(std::move(child));
}

void ParallelEye::refit(Circle &c, const Bitmap3 &img) const
{
    vector<Circle> votes;
    for (const auto &child : children) {
        votes.push_back(c);
        child->refit(votes.back(), img);
        ///@todo if any subset has estimated stddev < 1px, return its average
    }
    c = Circle::average(votes);
}

void SerialEye::add(FindEyePtr&& child)
{
    children.emplace_back(std::move(child));
}

void SerialEye::refit(Circle &c, const Bitmap3 &img) const
{
    for (const auto &child : children) {
        child->refit(c, img);
    }
}

inline void cast_vote(Bitmap1 &img, Vector2 v, float weight)
{
    Vector2 pos = img.to_local(v);
    const int left = pos(0), right = left + 1, y = pos(1);
    if (left < 0 or right >= img.cols or y < 0 or y >= img.rows) {
        return;
    }
    float* row = img.ptr<float>(y);
    const float lr = pos(0) - left;
    row[left] += weight * (1 - lr);
    row[right] += weight * lr;
}

void HoughEye::refit(Circle &c, const Bitmap3 &img) const
{
    const float max_distance = 2 * c.radius;
    const Vector2 span(max_distance, max_distance);
    Region region(c.center - span, c.center + span);
    Bitmap1 votes(to_rect(region));
    votes = 0;
    Bitmap1 gray = img.grayscale(region);
    if (not gray.rows or not gray.cols) {
		printf(" empty bitmap around eye (%g, %g)\n", c.center[0], c.center[1]);
        return;
    }
    Bitmap1 dx = gray.d(0), dy = gray.d(1);
    for (Pixel p : dx) {
        Vector2 v = dx.to_world(p);
        Vector2 gradient(dx(p), dy(v));
        float size = cv::norm(gradient);
        cast_vote(votes, v - gradient * c.radius / size, size);
    }
    for (Pixel p : dy) {
        Vector2 v = dy.to_world(p);
        Vector2 gradient(dx(v), dy(p));
        float size = cv::norm(gradient);
        cast_vote(votes, v - gradient * c.radius / size, size);
    }
    Pixel result;
    c.center = precise_maximum(votes);
}

void LimbusEye::refit(Circle &c, const Bitmap3 &img) const
{
    const int iteration_count = 5;
    const Vector2 margin = Vector2(1, 1) * (iteration_count + c.radius);
    Region bounds(c.center - margin, c.center + margin);
    Bitmap1 gray = img.grayscale(bounds);
    if (not gray.rows or not gray.cols) {
        return;
    }
    Bitmap1 dxx = gray.d(0).d(0), dxy = gray.d(0).d(1), dyy = gray.d(1).d(1);
    for (int iteration=0; iteration < iteration_count; iteration++) {
        float weight = (2 * iteration < iteration_count) ? 1 : 1.f / (1 << (iteration / 2 - iteration_count / 4));
        Vector2 delta_pos = {dp(c, 0, dxx) + dp(c, 1, dxy), dp(c, 0, dxy) + dp(c, 1, dyy)};
        float length = std::max({std::abs(delta_pos(0)), std::abs(delta_pos(1)), 1e-5f});
        float step = (1.f / (1 << iteration)) / length;
        c.center += step * delta_pos;
    }
}

float LimbusEye::dp(Circle c, int direction, const Bitmap1 &derivative) const
{
    float result = 0;
    float sum_weight = 0;
    for (Pixel p : derivative) {
        Vector2 offcenter = derivative.to_world(p) - c.center;
        float w = 1 - std::abs(cv::norm(offcenter) - c.radius);
        if (w > 0) {
            result += w * (derivative(p) * offcenter(direction)) / c.radius;
            sum_weight += w;
        }
    }
    return sum_weight > 0 ? result / sum_weight : 0;
}


