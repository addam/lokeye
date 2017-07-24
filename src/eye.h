#ifndef EYE_H
#define EYE_H
#include <memory>
#include "main.h"
#include "bitmap.h"

class FindEye
{
public:
    virtual void refit(Circle&, const Bitmap3&) const = 0;
};
using FindEyePtr = std::unique_ptr<FindEye>;

/** Eye localizer that combines several other algorithms by voting
 */
class ParallelEye : public FindEye
{
    vector<FindEyePtr> children;
public:
    virtual void refit(Circle&, const Bitmap3&) const;
    void add(FindEyePtr&&);
};

/** Eye localizer that runs several other algorithms in sequence
 */
class SerialEye : public FindEye
{
    vector<FindEyePtr> children;
public:
    virtual void refit(Circle&, const Bitmap3&) const;
    void add(FindEyePtr&&);
};

/** Subpixel Hough circle detector with fixed radius
 */
class HoughEye : public FindEye
{
public:
    virtual void refit(Circle&, const Bitmap3&) const;
};

/** Maximize sum of gradient towards the eye center
 */
class LimbusEye : public FindEye
{
    /// Derivative of the energy function wrt. center coordinate `direction`
    float dp(Circle, int direction, const Bitmap1&) const;
public:
    virtual void refit(Circle&, const Bitmap3&) const;
};

/** Normalized correlation with a circle
 */
class CorrelationEye : public FindEye
{
public:
    virtual void refit(Circle&, const Bitmap3&) const;
};    

/** Model based on radial symmetry of the iris and pupil
 */
class RadialEye : public FindEye
{
    bool use_color_median;
    float eval(Circle, const Bitmap3&, const Bitmap1&, const Bitmap1&) const;
    vector<Vector3> radial_mean(Circle, const Bitmap3&) const;
    static float grad_func(Vector2, Vector2);
public:
    RadialEye(bool use_color_median=true) : use_color_median(use_color_median) {}
    virtual void refit(Circle&, const Bitmap3&) const;    
};
#endif // EYE_H
