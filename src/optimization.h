#ifndef OPTIMIZATION_H
#define OPTIMIZATION_H
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)
#include STRINGIFY(TSF_HEADER)
#include STRINGIFY(CHL_HEADER)
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
    
    Children children;
    Face(const Bitmap3 &ref, Region, Circle, Circle);
    Vector3 update_step(const Bitmap3 &img, const Bitmap3 &grad, const Bitmap3 &reference, int direction) const;
    void refit(const Bitmap3&, bool only_eyes=false);
    Vector4 operator() () const;
    void render(const Bitmap3&, const char*) const;
};

void refit_transformation(Transformation&, Region, const Bitmap3&, const Bitmap3&, int min_size=3);
Face init_interactive(const Bitmap3&);
Face init_static(const Bitmap3&, const string &face_xml="/usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml", const string &eye_xml="/usr/local/share/OpenCV/haarcascades/haarcascade_eye_tree_eyeglasses.xml");
Gaze calibrate_interactive(Face&, VideoCapture&, Pixel window_size=Pixel(1400, 700));
Gaze calibrate_static(Face&, VideoCapture&, TrackingData::const_iterator&);

#endif
