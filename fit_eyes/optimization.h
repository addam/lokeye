#ifndef OPTIMIZATION_H
#define OPTIMIZATION_H
#include TSF_HEADER

using Measurement = std::pair<Vector4, Vector2>;

struct Gaze
{
    Matrix35 fn;
    Gaze(const vector<Measurement>&, int &out_support, float precision=50);
    Vector2 operator () (Vector4) const;
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
    /** Regions in reference space
     */
    Region main_region;
    Region eye_region;
    Region nose_region;
    
    /** Eyes in eye space
     */
    std::array<Eye, 2> eyes;
    
    /** Reference image
     */
    Bitmap3 ref;
    
    /** Transformation from reference to view space
     */
    Transformation main_tsf;
    Transformation eye_tsf;
    Transformation nose_tsf;
    
    Matrix eigenfaces;
    Matrix subspace;
    
    Face(const Bitmap3 &ref, Region main, Region eye, Region nose, Eye, Eye);
    Vector3 update_step(const Bitmap3 &img, const Bitmap3 &grad, const Bitmap3 &reference, int direction) const;
    void refit(const Bitmap3&, bool only_eyes=false);
    Vector4 operator() () const;
    void render(const Bitmap3&) const;
};

Face mark_eyes(Bitmap3&);
Gaze calibrate(Face&, VideoCapture&, Pixel window_size=Pixel(1400, 700));

#endif
