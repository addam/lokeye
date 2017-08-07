#ifndef TRANSFORMATION_BARYCENTRIC_H
#define TRANSFORMATION_BARYCENTRIC_H

struct Transformation
{
    using Params = Matrix23;
    Triangle region;
    Params points;  /// Transformation from barycentric to view coordinates
    const Matrix33 static_params;  /// Transformation from homogeneous reference to barycentric coordinates
    Params params;  /// Transformation from homogeneous reference to view coordinates; params == points * static_params
    
    Transformation();
    Transformation(Region region);
    Transformation(Triangle);
    Transformation(Triangle, decltype(params), decltype(static_params));
    Transformation& operator = (const Transformation&);
    Transformation operator + (const Params&) const;
    Transformation& operator += (const Params&);
    Transformation& increment(Vector2, int);
    Vector2 operator () (Vector2) const;
    Vector2 operator () (Pixel p) const { return (*this)(to_vector(p)); }
    Region operator () (Region) const;
    Triangle operator () (Triangle) const;
    Transformation operator () (Transformation) const;
    
    float scale(Vector2) const;
    Vector2 operator - (const Transformation&) const;
    Transformation inverse() const;
    Vector2 inverse(Vector2 v) const { return inverse()(v); }
    Transformation inverse(Transformation) const;
    Params d(Vector2, int direction) const;
    Triangle vertices() const;
protected:
    void update_params(); /// recalculate params after a modification of points
};

Vector2 extract_point(const Transformation&, unsigned);
Vector2 extract_point(const Transformation::Params&, unsigned);
#endif
