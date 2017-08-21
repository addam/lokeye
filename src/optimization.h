#ifndef OPTIMIZATION_H
#define OPTIMIZATION_H
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)
#include STRINGIFY(TSF_HEADER)
#include STRINGIFY(CHL_HEADER)
#include "bitmap.h"
#include "eye.h"
#include "system_paths.h"

using Measurement = std::pair<Vector4, Vector2>;

class Gaze
{
    Matrix35 fn;
    Gaze(const Matrix35&);
public:
    static Gaze ransac(const vector<Measurement>&, int &out_support, float precision=50);
    static Gaze guess(Safe<vector<Measurement>>&, int &out_support, float precision=50);
    Vector2 operator () (Vector4) const;
};

struct Face
{
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

void refit_transformation(Transformation&, const Bitmap3&, const Bitmap3&, int min_size=3);
Face init_interactive(const Bitmap3&);
Face init_static(const Bitmap3&, const string &face_xml=face_classifier_xml, const string &eye_xml=eye_classifier_xml);
Gaze calibrate_interactive(Face&, VideoCapture&, Pixel window_size=Pixel(1400, 700));
Gaze calibrate_static(Face&, VideoCapture&, TrackingData::const_iterator&);

float line_search(Transformation::Params, float &prev_energy, float max_length, const Transformation&, const Bitmap3&, const Bitmap3&);
float step_length(Transformation::Params, const Transformation&);
float evaluate(const Transformation&, const Bitmap3&, const Bitmap3&);
Transformation::Params update_step(const Transformation&, const Bitmap3&, const Bitmap3&, const Bitmap3&, int direction);
#endif
