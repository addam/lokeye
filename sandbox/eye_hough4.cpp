#include "../src/main.h"
#include "../src/bitmap.h"

Bitmap3 img;

struct Sample
{
	Vector2 center = {0, 0};
	float weight = 0;
	Vector2 origin = {0, 0};
};

struct ListMap
{
	unsigned rows, cols;
	unsigned shift;
	vector<vector<Sample>> samples;
	unsigned cell(Vector2 v);
	ListMap(unsigned rows, unsigned cols, unsigned shift) :
		rows(rows), cols(cols), shift(shift),
		samples((rows >> shift) * (cols >> shift)) {
	}
};

class VotingSpace
{
	const int min_size = 4;
	vector<ListMap> layers;
	void cast(Sample);
public:
	VotingSpace(int rows, int cols);
	ListMap &layer(float scale);
	void cast(Vector2 v, Vector2 gradient, float radius);
	Pixel maximum() const;
	Bitmap1 flatten() const;
};

float pow3(float x)
{
	return x * x * x;
}

Sample average(const vector<Sample> &samples)
{
	Sample result;
	float coef = 1.0 / samples.size();
	for (Sample s : samples) {
		result.center += s.center * coef;
		/// @todo tady se musejí nějak zprůměrovat poloměry
		result.weight += s.weight;
	}
	return result;
}

VotingSpace::VotingSpace(int rows, int cols) : rows(rows), cols(cols)
{
    while (rows >= min_size and cols >= min_size) {
		layers.emplace_back(rows, cols);
		rows /= 2, cols /= 2;
	}
	samples.resize(size);
}

vector<Sample>& ListMap::cell(Vector2 v)
{
	unsigned row = unsigned(v[1]) >> shift, column = unsigned(v[0]) >> shift;
	return samples.at(row * cols + column);
}

ListMap& VotingSpace::layer(float scale)
{
	return layers.at((radius > 1) ? std::log2f(radius) : 0);
}

void VotingSpace::cast(Sample s)
{
}

void VotingSpace::cast(Vector2 v, Vector2 gradient, float radius)
{
	float size = cv::norm(gradient);
	if (size > 0) {
		Vector2 center = v + radius * gradient / size;
		layer(radius).cell(center).emplace_back({center, size, v});
	}
}

Sample VotingSpace::maximum() const
{
	Sample result;
	for (const ListMap &layer : layers) {
		for (const vector<Sample> &block : layer.samples) {
			if (not block.empty() and average(block).weight > result.weight) {
				result = average(block);
			}
		}
	}
	return result;
}

Bitmap1 VotingSpace::flatten() const
{
	Bitmap1 result(rows, cols);
	unsigned max = 0;
	for (Pixel p : result) {
		Vector2 v = result.to_world(p);
		unsigned sum;
		for (const ListMap &layer : layers) {
			sum += layer.sum_weight(v);
		}
		result(p) = sum;
		max = std::max(max, sum);
	}
	result *= 1.0 / max;
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
        if (gradient != {0, 0}) {
	        float radius = - pow3(size) / (dxx(v) * pow2(gradient[1]) + dyy(v) * pow2(gradient[0]) - 2 * dxy(v) * gradient[0] * gradient[1]);
	        votes.cast(v, gradient, radius);
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
