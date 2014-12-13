#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <cstdio>

typedef unsigned char uchar;
const char *winname = "image";

float pow2(float x);

struct Circle
{
    float x, y, r;
    Circle() = default;
    Circle(float _x, float _y, float _r):
        x(_x), y(_y), r(_r)
    {
    }
    void draw(cv::Mat canvas, cv::Scalar _color) const
    {
        cv::Vec3b color(_color[0], _color[1], _color[2]);
        for (int row = MAX(y - r, 0); row < y + r + 2 && row < canvas.rows; row++) {
            cv::Vec3b *img_row = canvas.ptr<cv::Vec3b>(row);
            for (int col = MAX(x - r, 0); col < x + r + 2 && col < canvas.cols; col++) {
                float w = 1 - std::abs(std::sqrt(pow2(row - y) + pow2(col - x)) - r);
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

static void onMouse(int event, int x, int y, int, void* param)
{
	if (event == cv::EVENT_LBUTTONDOWN or event == cv::EVENT_RBUTTONDOWN) {
        int best_r = 15;
        float best_val = -1e30;
        for (int r = 5; r < 20; r++) {
            Circle c(x, y, r);
            float val = eval(c, g, h);
            if (val > best_val) {
                best_val = val;
                best_r = r;
            }
        }
        if (event == cv::EVENT_LBUTTONDOWN) {
            left = Circle(x, y, best_r);
            optimize(left);
        } else if (event == cv::EVENT_RBUTTONDOWN) {
            right = Circle(x, y, best_r);
            optimize(right);
        }
        cv::Mat &img = *(cv::Mat*)param;
        cv::Mat canvas = img.clone();
        left.draw(canvas, cv::Scalar(0,0,255));
        right.draw(canvas, cv::Scalar(0,255,0));
        cv::imshow(winname, canvas);
    } else if (event == cv::EVENT_RBUTTONDOWN) {
        right = Circle(x, y, 10);
    }
}

void process(char* filename)
{
    cv::Mat img = cv::imread(filename);
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
    
    cv::imshow(winname, img);
    cv::setMouseCallback(winname, onMouse, &img);
    cv::waitKey();
    printf("%4.2f %4.2f %4.2f %4.2f %4.2f %4.2f %s\n", left.x, left.y, left.r, right.x, right.y, right.r, filename);
}

int main(int argc, char* argv[])
{
    const char
        rows[] = {'a', 'b', 'c'},
        columns[] = {'1', '2', '3', '4'};
    char filename[] = "dataset/a1a1.jpg";
    for (char hr : rows) {
        filename[8] = hr;
        for (char hc : columns) {
            filename[9] = hc;
            for (char er : rows) {
                filename[10] = er;
                for (char ec : columns) {
                    filename[11] = ec;
                    process(filename);
                }
            }
        }
    }
    
    return EXIT_SUCCESS;
}
