#include "../src/main.h"
#include "../src/bitmap.h"

Bitmap3 img;

class VotingSpace
{
	const int min_size = 4;
	vector<Bitmap1> layers;
	void single_cast(Vector2 v, unsigned layer, float weight);
public:
	VotingSpace(int rows, int cols);
	void cast(Vector2 v, float scale, float weight);
	Pixel maximum() const;
	Bitmap1 flatten() const;
};

float pow3(float x)
{
	return x * x * x;
}

VotingSpace::VotingSpace(int rows, int cols)
{
	layers.push_back(Bitmap1(rows, cols));
	layers.back() = 0;
    while (layers.back().rows > min_size and layers.back().cols > min_size) {
		layers.push_back(layers.back().downscale());
	}
}

void VotingSpace::single_cast(Vector2 v, unsigned layer, float weight)
{
	Bitmap1 &img = (layer < layers.size()) ? layers[layer] : layers.back();
    Vector2 pos = img.to_local(v);
    const int left = pos(0), right = left + 1, top = pos(1), bottom = top + 1;
    if (left < 0 or right >= img.cols or top < 0 or bottom >= img.rows) {
        return;
    }
    float *row_top = img.ptr<float>(top), *row_bottom = img.ptr<float>(bottom);
    const float lr = pos(0) - left, ud = pos(1) - top;
    row_top[left] += weight * (1 - lr) * (1 - ud);
    row_top[right] += weight * lr * (1 - ud);
    row_bottom[left] += weight * (1 - lr) * ud;
    row_bottom[right] += weight * lr * ud;
}

void VotingSpace::cast(Vector2 v, float scale, float weight)
{
	float layer = (scale > 1) ? std::log2f(scale) : 0;
	unsigned layer_idx = layer;
	float t = layer - layer_idx;
	single_cast(v, layer_idx, (1 - t) * weight);
	single_cast(v, layer_idx + 1, t * weight);
}

Pixel VotingSpace::maximum() const
{
    Pixel result;
    cv::minMaxLoc(layers.front(), NULL, NULL, NULL, &result);
    return result;
}

Bitmap1 VotingSpace::flatten() const
{
	Bitmap1 result = layers.front().clone();
	for (Pixel p : result) {
		float weight = 1;
		Vector2 v = result.to_world(p);
		for (size_t i=1; i<layers.size(); ++i) {
			weight *= 0.5; // should probably be 0.25, but that's just too noisy
			// anyway, there is some mess with the gradient-magnitude weighting, so this might be correct
			result(p) += weight * layers[i](v);
		}
	}
	return result;
}

bool local_maximum(const Bitmap1 &img, Pixel p)
{
	int x = p.x, y = p.y;
	return ((p.x == 0 or img(p) >= img(Pixel(x - 1, y))) and
		(p.x == img.cols - 1 or img(p) >= img(Pixel(x + 1, y))) and
		(p.y == 0 or img(p) > img(Pixel(x, y - 1))) and
		(p.y == img.rows - 1 or img(p) > img(Pixel(x, y + 1))));
}

Bitmap1 circles(const Bitmap1 &gray)
{
	// curvature = - (f_xx * f_y**2 + f_yy * f_x**2 - 2*f_xy*f_x*f_y) / (f_x**2 + f_y**2)**(3/2)
    VotingSpace votes(gray.rows, gray.cols);
    Bitmap1 dx = gray.d(0), dy = gray.d(1);
    Bitmap1 dxx = dx.d(0), dxy = dx.d(1), dyy = dy.d(1);
    for (Pixel p : gray) {
        Vector2 v = gray.to_world(p);
        Vector2 gradient(dx(v), dy(v));
        float size = cv::norm(gradient);
        if (size != 0) {
	        float radius = - pow3(size) / (dxx(v) * pow2(gradient[1]) + dyy(v) * pow2(gradient[0]) - 2 * dxy(v) * gradient[0] * gradient[1]);
	        if (radius < 0) {
		        votes.cast(v + radius * gradient / size, std::abs(radius), size);
		        const int grid = 10;
		        if (std::abs(radius) < 1000 and p.x % grid == 0 and p.y % grid == 0) {
					//if (radius != 0) {
						//printf("%i, %i radius: %g -(%g * %g**2 + %g * %g**2 - 2 * %g * %g * %g / pow3(%g)\n", p.x, p.y, radius, dxx(v), gradient[1], dyy(v), gradient[0], dxy(v), gradient[0], gradient[1], size);
					//}
					cv::line(img, p, to_pixel(gray.to_local(v + radius * gradient / size)), cv::Scalar(0, 1, 0));
					cv::line(img, p, to_pixel(gray.to_local(v - 2 * gradient / size)), cv::Scalar(0, 0, 1));
				}
			}
		}
    }
    Bitmap1 result = votes.flatten();
	Pixel maximum;
	double ceiling;
    cv::minMaxLoc(result, NULL, &ceiling, NULL, &maximum);
    cv::circle(img, maximum, 10, cv::Scalar(0.5, 0, 1), 3);
    result *= 1 / ceiling;
    return result;
}

int main(int argc, char **argv)
{
	if (argc > 1) {
		img.read(argv[1]);
	} else {
	    VideoCapture cam = (argc > 1) ? VideoCapture{argv[1]} : VideoCapture{0};
		img.read(cam);
	}
    cv::blur(img, img, cv::Size(5, 5));
	Bitmap3 canvas(img.rows, 2 * img.cols);
	Bitmap1 milkyway = circles(img.grayscale());
	cv::cvtColor(milkyway, canvas.colRange(0, img.cols), cv::COLOR_GRAY2BGR);
    for (Pixel p : milkyway) {
	    if (local_maximum(milkyway, p)) {
			canvas(p) = Vector3(0, milkyway(p), 2 * milkyway(p));
		}
	}
	img.copyTo(canvas.colRange(img.cols, 2 * img.cols));
	cv::imshow("black circles", canvas);
	cv::waitKey();
}
