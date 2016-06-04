#ifndef OPTIMIZATION_H
#define OPTIMIZATION_H

struct Gaze
{
    Matrix33 fn;
    Gaze(const std::vector<std::pair<Vector2, Vector2>>&, int &out_support, float precision=50);
    Vector2 operator () (Vector2) const;
};

struct Eye
{
    /** Position in reference space
     */
    Vector2 pos;
    Vector2 init_pos;
    float radius;
    Eye(Vector2 pos, float radius) : pos{pos}, init_pos{pos}, radius{radius} {
    }
    void refit(const Bitmap3&, const Transformation&);
    void init(const Bitmap3&, const Transformation&);
protected:
    float sum_boundary_dp(const Bitmap1&, bool is_vertical, const Transformation&);
};

struct Face
{
    /** Region in reference space
     */
    Region region;
    
    std::array<Eye, 2> eyes;
    
    /** Reference image
     */
    Bitmap3 ref;
    
    /** Transformation from reference to view space
     */
    Transformation tsf;
    
    Face(const Bitmap3 &ref, Region region, Eye left_eye, Eye right_eye) : ref{ref.clone()}, tsf{region}, region{region}, eyes{left_eye, right_eye} {
    }
    void refit(const Bitmap3&, bool only_eyes=false);
    Vector2 operator() () const;
    void render(const Bitmap3&) const;
};

Face mark_eyes(Bitmap3&);
Gaze calibrate(Face&, VideoCapture&, Pixel window_size=Pixel(1400, 700));

#endif
