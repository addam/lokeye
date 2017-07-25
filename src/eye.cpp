#include "eye.h"
#include <iostream>

using Curve = std::vector<Pixel>;

namespace {

/** Get quadratic-interpolated maximum location in world coordinates
 */
Vector2 precise_maximum(Bitmap1 score)
{
	cv::Point origin;
	cv::minMaxLoc(score, 0, 0, 0, &origin);
	origin.x = clamp(origin.x, 1, score.cols - 2);
	origin.y = clamp(origin.y, 1, score.rows - 2);
	Matrix system, rhs;
	for (int x=-1; x<=1; ++x) {
		for (int y=-1; y<=1; ++y) {
            float s = score(Pixel(x, y) + origin);
            if (std::isfinite(s)) {
                Matrix tmp(1, 6);
                tmp(0) = x*x;
                tmp(1) = x*y;
                tmp(2) = x;
                tmp(3) = y*y;
                tmp(4) = y;
                tmp(5) = 1;
                system.push_back(tmp);
                rhs.push_back(s);
            }
		}
	}
	Matrix v;
	cv::solve(system, rhs, v, cv::DECOMP_SVD);
	Matrix polynomial(3, 3);
	polynomial << 2*v(0), v(1), v(2), v(1), 2*v(3), v(4), v(2), v(4), 2*v(5);
	Matrix result;
	cv::solve(polynomial.colRange(0, 2).rowRange(0, 2), polynomial.rowRange(0, 2).col(2), result);
    Vector2 offset(clamp(result(0), -1, 1), clamp(result(1), -1, 1));
    return score.to_world(origin) - offset;
}

Curve make_circle(int r)
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
	return result;
}

void error_message(Circle c)
{
    printf(" empty bitmap around eye (%g, %g)\n", c.center[0], c.center[1]);
}

} // end anonymous namespace

void ParallelEye::add(FindEyePtr&& child)
{
    children.emplace_back(std::move(child));
}

int subset_size(int mask)
{
    int result = 0;
    for (; mask; mask >>= 1) {
        result += mask & 1;
    }
    return result;
}

Vector2 subset_mean(const vector<Vector2> &points, int mask)
{
    Vector2 sum;
    for (int i = 1; i <= mask; i <<= 1) {
        if (i & mask) {
            sum += points[i];
        }
    }
    return sum / subset_size(mask);    
}

float subset_variance(const vector<Vector2> &points, int mask)
{
    Vector2 mean = subset_mean(points, mask);
    float variance;
    for (int i = 1; i <= mask; i <<= 1) {
        if (i & mask) {
            variance += cv::norm(points[i] - mean, cv::NORM_L2SQR);
        }
    }
    return variance / (subset_size(mask) - 1);
}

void ParallelEye::refit(Circle &circle, const Bitmap3 &img) const
{
    if (children.empty()) {
        return;
    } else if (children.size() == 1) {
        children.front()->refit(circle, img);
        return;
    }
    vector<Vector2> votes;
    for (const auto &child : children) {
        Circle copy = circle;
        child->refit(copy, img);
        votes.push_back(copy.center);
    }
    float best_variance = HUGE_VALF;
    for (int i=0; i < (1 << votes.size()); ++i) {
        if (subset_size(i) < 2) {
            continue;
        }
        float var = subset_variance(votes, i);
        if (var < best_variance) {
            circle.center = subset_mean(votes, i);
            best_variance = var;
        }
    }
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
        error_message(c);
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
    constexpr int iteration_count = 5;
    const Vector2 margin = Vector2(1, 1) * (iteration_count + c.radius);
    Region bounds(c.center - margin, c.center + margin);
    Bitmap1 gray = img.grayscale(bounds);
    if (not gray.rows or not gray.cols) {
        error_message(c);
        return;
    }
    Bitmap1 dxx = gray.d(0).d(0), dxy = gray.d(0).d(1), dyy = gray.d(1).d(1);
    for (int iteration=0; iteration < iteration_count; iteration++) {
        float weight = (2 * iteration < iteration_count) ? 1 : 1.f / (1 << (iteration / 2 - iteration_count / 4));
        Vector2 delta_pos = {dp(c, 0, dxx) + dp(c, 1, dxy), dp(c, 0, dxy) + dp(c, 1, dyy)};
        float length = std::max({std::abs(delta_pos(0)), std::abs(delta_pos(1)), 1e-5f});
        float step = (max_distance / (1 << iteration)) / length;
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
    Bitmap1 gray = img.grayscale(to_region(2 * scale * c));
    if (not gray.rows or not gray.cols) {
        error_message(c);
        return;
    }
    const Vector2 offset = Vector2(scale, scale) * c.radius;
	Matrix templ = Matrix::ones(2 * offset(1) + 1, 2 * offset(0) + 1);
    if (gray.rows < templ.rows or gray.cols < templ.cols) {
        return;
    }
	cv::circle(templ, to_pixel(offset), c.radius, 0.0, -1);
    Bitmap1 score = gray.crop(to_region(scale * c)).clone();
	cv::matchTemplate(gray, templ, score, cv::TM_CCORR_NORMED);
	c.center = precise_maximum(score);
}

BitmapEye::BitmapEye(const std::string filename, float radius_scale, float neighborhood):
    radius_scale(radius_scale),
    scale(neighborhood)
{
    cv::Mat tmp = cv::imread(filename, cv::IMREAD_UNCHANGED);
    tmp.convertTo(tmp, CV_32F, 1./255);
    cv::split(tmp, templ);
    assert(templ.size() >= 3);
}

void BitmapEye::refit(Circle &c, const Bitmap3 &img) const
{
    Bitmap3 square = img.crop(to_region(scale * c));
    int radius = c.radius / radius_scale;
    cv::Size size(2 * radius + 1, 2 * radius + 1);
    if (square.rows < size.height or square.cols < size.width) {
        return;
    }
    /// @todo these calculations work only when img.scale == 1
    Bitmap1 score(square.rows - size.height + 1, square.cols - size.width + 1, c.center - to_vector(size / 2));
    vector<Matrix> square_channels;
    cv::split(square, square_channels);
    Matrix mask;
    if (templ.size() > 3) {
        cv::resize(templ[3], mask, size);
    }
	for (int i=0; i<3; ++i) {
        Matrix score_channel, templ_channel;
        cv::resize(templ[i], templ_channel, size);
        matchTemplate(square_channels[i], templ_channel, score_channel, cv::TM_CCORR_NORMED, mask);
        score += score_channel;
    }
    Pixel pmax;
	cv::minMaxLoc(score, 0, 0, 0, &pmax);
    Vector2 discrete = score.to_world(pmax);
    Vector2 interp = precise_maximum(score);
    if (std::abs(discrete[0] - interp[0]) + std::abs(discrete[1] - interp[1]) < 2) {
        c.center = interp;
    } else {
        c.center = discrete;
    }
}

/// cheap (and shifted) approximation to atan2
int RadialEye::angle_address(Vector2 v, int bin_count)
{
    float x = v[0], y = v[1];
    float angle = (std::abs(x) >= std::abs(y)) ? (y / x + (x > 0) ? 1 : 5) : (-x / y + (y > 0) ? 3 : 7);
    assert (angle >= 0 and angle < 8);
	return angle * bin_count / 8;
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

vector<Vector3> RadialEye::radial_mean(Circle c, const Bitmap3 &img) const
{
    vector<vector<Vector3>> histogram(c.radius + 1);
    for (Pixel p : img) {
        Vector2 diff = img.to_world(p) - c.center;
        int r = cv::norm(diff);
        if (r < c.radius) {
            histogram[r].push_back(img(p));
        }
	}
	vector<Vector3> result;
    for (vector<Vector3> &bin : histogram) {
        if (use_color_median) {
            Vector3 best(0, 0, 0);
            float best_cost = HUGE_VALF;
            for (Vector3 anchor : bin) {
                float cost = 0;
                for (Vector3 other : bin) {
                    cost += cv::norm(anchor - other);
                }
                if (cost < best_cost) {
                    best = anchor;
                    best_cost = cost;
                }
            }
            result.push_back(best);
        } else {
            if (bin.empty()) {
                result.emplace_back(0, 0, 0);
            } else {
                auto mid = bin.begin() + bin.size() / 2;
                std::nth_element(bin.begin(), mid, bin.end(), [](Vector3 l, Vector3 r) {
                    return cv::norm(l) < cv::norm(r);
                });
                result.push_back(*mid);
            }
        }
    }
    return result;
}

float RadialEye::eval(Circle c, const Bitmap3 &img, const Bitmap1 &dx, const Bitmap1 &dy) const
{
    const int angular_bins = 2 * c.radius * M_PI;
	// calculate average over each radius
	vector<Vector3> colors = radial_mean(c, img);
	// calculate score of each angle, based on the gradient at its end
	std::vector<float> limbus_score(angular_bins);
    const Curve circle = make_circle(c.radius);
    for (Pixel d : circle) {
        Vector2 diff = to_vector(d);
        Vector2 v = c.center + diff;
        if (img.contains(v)) {
            int index = angle_address(diff, angular_bins);
            limbus_score[index] = grad_func(Vector2(dx(v), dy(v)), diff);
        }
	}
	std::vector<float> iris_score(angular_bins, 0);
	std::vector<float> iris_weight(angular_bins, 0);
	for (Pixel p : img) {
        Vector2 diff = img.to_world(p) - c.center;
        int r = cv::norm(diff);
        if (r <= c.radius and r != 0) {
            float rs = std::max(0.0, 1 - cv::norm(colors[r] - img(p)) / 20);
            int index = angle_address(diff, angular_bins);
            iris_score[index] += rs;
            iris_weight[index] += 1;
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
    if (not gray.rows or not gray.cols) {
        error_message(c);
        return;
    }
    const Bitmap1 dx = gray.d(0), dy = gray.d(1);
	Bitmap1 score = gray.crop(to_region(c));
    for (Pixel p : score) {
        Circle candidate{score.to_world(p), c.radius};
        Bitmap3 cropout = img.crop(to_region(candidate));
        score(p) = eval(candidate, cropout, dx, dy);
	}
	c.center = precise_maximum(score);
}
