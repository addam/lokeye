#include "eye.h"

using Curve = std::vector<Pixel>;

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

namespace {

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

Curve make_circle(int r, bool do_shift=false)
{
	Curve result;
	for (int x=0; ; ++x) {
		int y = sqrt(pow2(r) - pow2(x));
		if (y <= x) {
			break;
		} else {
			result.push_back(Pixel(x, y));
		}
	}
	result.reserve(8 * result.size() - 4);
	std::transform(result.begin(), result.end() - (result.back().x == result.back().y), back_inserter(result), [](Pixel p) { return Pixel(p.y, p.x); });
	std::transform(result.begin(), result.end() - 1, back_inserter(result), [](Pixel p) { return Pixel(p.x, -p.y); });
	std::transform(result.begin() + 1, result.end() - 1, back_inserter(result), [](Pixel p) { return Pixel(-p.x, p.y); });
	std::sort(result.begin(), result.end(), [](Pixel a, Pixel b) { return a.y < b.y or (a.y == b.y and a.x < b.x); });
    if (do_shift) {
        for (Pixel &p : result) {
            p.x += r;
            p.y += r;
        }
    }
	return result;
}

} // end anonymous namespace

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

void CorrelationEye::refit(Circle &c, const Bitmap3 &img) const
{
    ///@todo use a scale member for this
    const Vector2 offset = Vector2(1, 1) * 1.5 * c.radius;
	Matrix templ = Matrix::ones(2 * offset(1), 2 * offset(0));
	cv::circle(templ, to_pixel(offset), c.radius, 0.0, -1);
    Bitmap1 gray = img.grayscale(Region(c.center - 2 * offset, c.center + 2 * offset));
    Bitmap1 score = gray.crop(Region(c.center - offset, c.center + offset)).clone();
	cv::matchTemplate(gray, templ, score, cv::TM_CCORR_NORMED);
	c.center = precise_maximum(score);
}

template<typename T>
T &angle_address(std::vector<T> &seq, cv::Vec2f v)
{
    /// @todo this should scale with 'angular bins' from RadialEye::eval
	assert (v[0] != 0 or v[1] != 0);
	int index = seq.size() * atan2(v[1], v[0]) / (2 * M_PI);
	return seq.at((index + seq.size()) % seq.size());
}

template<typename T>
struct Histogram
{
	T mean;
	//int size;
	std::vector<T> data;
	void insert(const T&);
};

template<typename T>
void Histogram<T>::insert(const T &value)
{
	data.push_back(value);
	auto mid = data.begin() + data.size() / 2;
	std::nth_element(data.begin(), mid, data.end(), [](T l, T r) { return cv::norm(l) < cv::norm(r); });
	mean = *mid;
	//mean = mean * (size / float(size + 1)) + value / float(size + 1);
	//size += 1;
}

float RadialEye::grad_func(Vector2 grad, Vector2 direction)
{
	if (direction[0] == 0 and direction[1] == 0) {
		return 0;
	}
	float n = cv::norm(grad);
	float d = grad.dot(direction) / (n * cv::norm(direction));
	return (d > 0) ? pow2(pow2(d)) * n : 0;
}

float RadialEye::eval(Circle c, const Bitmap3 &img, const Bitmap1 &dx, const Bitmap1 &dy) const
{
    const int radial_bins = c.radius, angular_bins = 2 * c.radius * M_PI;
	// calculate average over each radius
	std::vector<Histogram<Vector3>> circles(radial_bins);
    const Bitmap3 square = img.crop(to_region(c));
    for (Pixel p : square) {
        Vector2 diff = square.to_world(p) - c.center;
        int r = cv::norm(diff);
        if (r < radial_bins) {
            circles[r].insert(square(p));
        }
	}
	// calculate score of each angle, based on the gradient at its end
	std::vector<float> limbus_score(angular_bins);
    const Curve circle = make_circle(c.radius, true);
    for (Pixel p : circle) {
        Vector2 v = square.to_world(p);
        Vector2 diff = v - c.center;
        angle_address(limbus_score, diff) = grad_func(Vector2(dx(v), dy(v)), diff);
	}
	std::vector<float> iris_score(angular_bins, 0);
	std::vector<float> iris_weight(angular_bins, 0);
	for (Pixel p : square) {
        Vector2 diff = square.to_world(p) - c.center;
        int r = cv::norm(diff);
        if (r < radial_bins and r != 0) {
            float rs = std::max(0.0, 1 - cv::norm(circles[r].mean - square(p)) / 20);
            angle_address(iris_score, diff) += rs;
            angle_address(iris_weight, diff) += 1;
        }
	}
	float result = 0;
	for (int i=0; i<angular_bins; ++i) {
		if (iris_weight[i] > 0) {
			result += limbus_score[i] * pow2(iris_score[i] / iris_weight[i]);
		}
	}
	return result;
}

void RadialEye::refit(Circle &c, const Bitmap3 &img) const
{
    Bitmap1 gray = img.grayscale(to_region(2 * c));
    const Bitmap1 dx = gray.d(0), dy = gray.d(1);
	Bitmap1 score = gray.crop(to_region(c));
    for (Pixel p : score) {
        score(p) = eval(Circle{score.to_world(p), c.radius}, img, dx, dy);
	}
	c.center = precise_maximum(score);
}
