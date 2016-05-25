#ifndef OPTIMIZATION_H
#define OPTIMIZATION_H

struct Gaze
{
    Matrix3 fn;
    Gaze(const std::vector<std::pair<Vector2, Vector2>>&, int &out_support, float precision=50);
    Vector2 operator () (Vector2) const;
};

struct Eye
{
    Vector2 pos;
    Vector2 init_pos;
    float radius;
    Eye(Vector2 pos, float radius) : pos{pos}, init_pos{pos}, radius{radius} {
    }
    void refit(Image&, const Transformation&);
protected:
    float sum_boundary_dp(const Bitmap3 &img_x, const Bitmap3 &img_y, const Transformation&);
    float sum_boundary_dr(Image&, const Transformation&);
    Iterrect region(const Transformation&) const;
};

struct Face
{
    Iterrect region;
    std::array<Eye, 2> eyes;
    Image ref;
    Transformation tsf;
    Face(const Image &ref, Rect region, Eye left_eye, Eye right_eye) : ref{ref}, tsf{region}, region{region}, eyes{left_eye, right_eye} {
    }
    void refit(Image&, bool only_eyes=false);
    Vector2 operator() () const;
    void render(const Image&) const;
};

Face mark_eyes(Image&);
Gaze calibrate(Face&, VideoCapture&, Pixel window_size=Pixel(1400, 700));

#endif
