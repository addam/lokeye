#ifndef OPTIMIZATION_H
#define OPTIMIZATION_H
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)
#include STRINGIFY(TSF_HEADER)
#include "bitmap.h"
#include "eye.h"

using Measurement = std::pair<Vector4, Vector2>;

struct Gaze
{
    Matrix35 fn;
    Gaze(const vector<Measurement>&, int &out_support, float precision=50);
    Vector2 operator () (Vector4) const;
};

struct Face
{
    /** Regions in reference space
     */
    Region main_region;
    Region eye_region;
    Region nose_region;
    
    /** Eyes in main reference space
     */
    std::array<Circle, 2> eyes;
    std::array<Circle, 2> fitted_eyes;
    FindEyePtr eye_locator;
    
    /** Reference image
     */
    Bitmap3 ref;
    
    /** Transformation from reference to view space
     */
    Transformation main_tsf;
    Transformation eye_tsf;
    Transformation nose_tsf;
    
    Face(const Bitmap3 &ref, Region main, Region eye, Region nose, Circle, Circle);
    Vector3 update_step(const Bitmap3 &img, const Bitmap3 &grad, const Bitmap3 &reference, int direction) const;
    void refit(const Bitmap3&, bool only_eyes=false);
    Vector4 operator() () const;
    void render(const Bitmap3&, const char*) const;
};

Face init_interactive(const Bitmap3&);
Face init_static(const Bitmap3&);
Gaze calibrate_interactive(Face&, VideoCapture&, Pixel window_size=Pixel(1400, 700));
Gaze calibrate_static(Face&, VideoCapture&, TrackingData::const_iterator&);

#endif
