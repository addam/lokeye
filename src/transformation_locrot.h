#ifndef TRANSFORMATION_LOCROT_H
#define TRANSFORMATION_LOCROT_H

struct Transformation
{
    using Params = Vector3;
    const std::pair<Vector2, float> static_params;
    std::pair<Vector2, float> params;
    Transformation();
    Transformation(Region region);
    Transformation(decltype(params), decltype(static_params));
    Transformation& operator = (const Transformation&);
    Transformation operator + (const Params&) const;
    Transformation& operator += (const Params&);
    Vector2 operator () (Vector2) const;
    Vector2 operator () (Pixel p) const { return (*this)(to_vector(p)); }
    Region operator () (Region) const;
    float scale(Vector2) const;
    Vector2 operator - (const Transformation&) const;
    Transformation inverse() const;
    Vector2 inverse(Vector2) const;
    Params d(Vector2, int direction) const;
protected:
    /// Calculate sin and cos, and return an internal scale coefficient
    float sincos(float &sin, float &cos) const;
};

#endif
