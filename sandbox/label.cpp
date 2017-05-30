#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <boost/filesystem.hpp>
#include <cstdio>
#include <string>
#include <algorithm>

namespace fs = boost::filesystem;
typedef unsigned char uchar;
const char *winname = "image";
int scale = 4;

float pow2(float x);

inline bool is_image_format(std::string ext)
{
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return ext == ".jpg" || ext == ".png" || ext == ".jpeg";
}

struct Circle
{
    float x, y, r=0;
    Circle() = default;
    Circle(float _x, float _y, float _r):
        x(_x), y(_y), r(_r)
    {
    }
    void draw(cv::Mat canvas, cv::Scalar _color) const
    {
		float sx = scale * x, sy = scale * y, sr = scale * r;
        cv::Vec3b color(_color[0], _color[1], _color[2]);
        for (int row = MAX(sy - sr, 0); row < sy + sr + 2 && row < canvas.rows; row++) {
            cv::Vec3b *img_row = canvas.ptr<cv::Vec3b>(row);
            for (int col = MAX(sx - sr, 0); col < sx + sr + 2 && col < canvas.cols; col++) {
                float w = 1 - std::abs(std::sqrt(pow2(row - sy) + pow2(col - sx)) - sr);
                if (w > 0) {
                    img_row[col] = w * color + (1-w) * img_row[col];
                }
            }
        }
    }
    void iadd(const float delta[3], float coef)
    {
        x += coef * delta[0];
        y += coef * delta[1];
        r += coef * delta[2];
    }
};

Circle left, right;

cv::Mat g, h, gx, gy, hy;

inline float pow2(float x)
{
    return x * x;
}

float eval(const Circle &c, const cv::Mat &img_x, const cv::Mat &img_y)
{
    const int
        top = MAX(c.y - c.r, 0),
        bottom = MIN(c.y + c.r + 2, img_x.rows),
        left = MAX(c.x - c.r, 0),
        right = MIN(c.x + c.r + 2, img_x.cols);
    float result = 0;
    float sum_weight = 0;
    for (int row = top; row < bottom; row++) {
        float dist_y = row - c.y;
        const float
            *g_row = img_x.ptr<float>(row),
            *h_row = img_y.ptr<float>(row);
        for (int col = left; col < right; col++) {
            float dist_x = col - c.x;
            float w = 1 - std::abs(std::sqrt(pow2(dist_x) + pow2(dist_y)) - c.r);
            if (w > 0) {
                result += w * (g_row[col] * dist_x + h_row[col] * dist_y) / c.r;
                sum_weight += w;
            }
        }
    }
    return result / sum_weight;
}

float dr(const Circle &c)
{
    const int
        top = MAX(c.y - c.r, 0),
        bottom = MIN(c.y + c.r + 2, g.rows),
        left = MAX(c.x - c.r, 0),
        right = MIN(c.x + c.r + 2, g.cols);
    float result = 0;
    float sum_weight = 0;
    for (int row = top; row < bottom; row++) {
        float dist_y = row - c.y;
        const float
            *gx_row = gx.ptr<float>(row),
            *gy_row = gy.ptr<float>(row),
            *hy_row = hy.ptr<float>(row);
        for (int col = left; col < right; col++) {
            float dist_x = col - c.x;
            float w = 1 - std::abs(std::sqrt(pow2(dist_x) + pow2(dist_y)) - c.r);
            if (w > 0) {
                result += w * (gx_row[col] * dist_x * dist_x + 2 * gy_row[col] * dist_x * dist_y + hy_row[col] * dist_y * dist_y) / pow2(c.r);
                sum_weight += w;
            }
        }
    }
    return result / sum_weight;
}

void optimize(Circle &c)
{
    const int iterations = 100;
    for (int i=0; i < iterations; i++) {
        float delta[] = {eval(c, gx, gy)/2, eval(c, gy, hy)/2, dr(c)/2};
        float step = 1. / std::max({std::abs(delta[0]), std::abs(delta[1]), std::abs(delta[2])});
        step /= MAX(1, step + 10 - iterations);
        float val = eval(left, g, h);
        #if 0
            printf("  eval: %g\n", val);
            printf("    step %g %g %g\n", delta[0]*step, delta[1]*step, delta[2]*step);
            float dif = 0.5;
            {
                Circle cdx(c.x+dif, c.y, c.r), cdy(c.x, c.y+dif, c.r), cdr(c.x, c.y, c.r+dif);
                printf("    fdif+ %g %g %g\n", (eval(cdx, g, h)-val)*step, (eval(cdy, g, h) - val)*step, (eval(cdr, g, h) - val)*step);
            }
            {
                Circle cdx(c.x-dif, c.y, c.r), cdy(c.x, c.y-dif, c.r), cdr(c.x, c.y, c.r-dif);
                printf("    fdif- %g %g %g\n", -(eval(cdx, g, h)-val)*step, -(eval(cdy, g, h) - val)*step, -(eval(cdr, g, h) - val)*step);
            }
        #endif
        c.iadd(delta, step);
    }
}

static void onMouse(int event, int sx, int sy, int flag, void* param)
{
	float x = float(sx) / scale, y = float(sy) / scale;
	static float begin_x, begin_y;
	static bool pressed = false;
	static bool disabled = false;
	if (event == cv::EVENT_LBUTTONDOWN or event == cv::EVENT_RBUTTONDOWN) {
		begin_x = x;
		begin_y = y;
		pressed = true;
		disabled = flag & cv::EVENT_FLAG_SHIFTKEY;
	} else if (pressed and flag & cv::EVENT_FLAG_SHIFTKEY) {
		if (flag & cv::EVENT_FLAG_LBUTTON) {
			left.x += x - begin_x;
			left.y += y - begin_y;
		} else if (flag & cv::EVENT_FLAG_RBUTTON) {
			right.x += x - begin_x;
			right.y += y - begin_y;
		}
		begin_x = x;
		begin_y = y;
		disabled = true;
	} else if (pressed and not disabled and (event == cv::EVENT_LBUTTONUP or event == cv::EVENT_RBUTTONUP)) {
		pressed = false;
        float r = std::hypot(x - begin_x, y - begin_y);
        if (event == cv::EVENT_LBUTTONUP) {
            left = Circle(begin_x, begin_y, r);
            optimize(left);
        } else if (event == cv::EVENT_RBUTTONUP) {
            right = Circle(begin_x, begin_y, r);;
            optimize(right);
        }
    } else {
		return;
	}
	cv::Mat &img = *(cv::Mat*)param;
	cv::Mat canvas = img.clone();
	left.draw(canvas, cv::Scalar(0,0,255));
	right.draw(canvas, cv::Scalar(0,255,0));
	cv::imshow(winname, canvas);
}

void process(fs::path filepath)
{
    cv::Mat img = cv::imread(filepath.native());
    cv::Mat gray;
    cv::cvtColor(img, gray, CV_BGR2GRAY);
    gray.convertTo(gray, CV_32F, 1./255);
    
    const int ksize = 3;
    const int type = CV_32FC1;
    const float normalization_1 = 1.0 / (1 << (2 * ksize - 3));
    const float normalization_2 = 1.0 / (1 << (2 * ksize - 4));
    cv::Sobel(gray, g, type, 1, 0, ksize, normalization_1);
    cv::Sobel(gray, h, type, 0, 1, ksize, normalization_1);
    cv::Sobel(gray, gx, type, 2, 0, ksize, normalization_2);
    cv::Sobel(gray, gy, type, 1, 1, ksize, normalization_2);
    cv::Sobel(gray, hy, type, 0, 2, ksize, normalization_2);
    
    //#define IM(name) cv::imshow(#name, name)
    //IM(g); IM(h); IM(gx); IM(gy); IM(hy);
    
    left.r = right.r = 0;
    scale = std::min(1600 / img.cols, 1000 / img.rows);
    cv::resize(img, img, cv::Size(), scale, scale, cv::INTER_NEAREST);
    cv::imshow(winname, img);
    cv::setMouseCallback(winname, onMouse, &img);
    while (char(cv::waitKey()) != ' ');
    printf("%s ", filepath.c_str());
    if (left.r > 0) {
	    printf("%4.2f %4.2f %4.2f ", left.x, left.y, left.r);
	}
	if (right.r > 0) {
	    printf("%4.2f %4.2f %4.2f", right.x, right.y, right.r);
	}
    printf("\n");
}

int main(int argc, char* argv[])
{
	for (int i=1; i<argc; ++i) {
		fs::path path(argv[i]);
		if (!fs::is_directory(path)) {
			process(path);
		} else {
			for (fs::directory_iterator it(path); it !=fs::directory_iterator(); ++it) {
		        if (!fs::is_directory(*it) && is_image_format(it->path().extension().native())) {
					process(*it);
				}
			}
		}
	}
    return EXIT_SUCCESS;
}
