#ifndef TRANSFORMATION_PERSPECTIVE_H
#define TRANSFORMATION_PERSPECTIVE_H

struct Transformation
{
    using Params = Vector8;
    using PointPack = array<Vector2, 4>;
    const PointPack static_params;  /// Arbitrary set of anchor points
    Matrix33 params;  /// Current homography matrix
    PointPack points;  /// Points from static_params transformed by the current homography
    const array<Matrix33, 4> canonize_matrix;  /// Homography for static_params into canonical configuration for their four permutations
    array<Vector3, 4> weight_vector;  /// Scale parameter of the decanonization matrices for four permutations of points
    array<array<Matrix33, 2>, 4> derivative_matrix;  /// Derivative of decanonization for two directions and four permutations
    Transformation();
    Transformation(Region region);
    Transformation(decltype(params), decltype(static_params));
    Transformation& operator = (const Transformation&);
    Transformation operator + (Params) const;
    Transformation& operator += (Params);
    Transformation& increment(Vector2, int);
    Vector2 operator () (Vector2) const;
    Vector2 operator () (Pixel p) const { return (*this)(to_vector(p)); }
    Region operator () (Region) const;
    Transformation operator () (Transformation) const;
    float scale(Vector2) const;
    Vector2 operator - (const Transformation&) const;
    Transformation inverse() const;
    Vector2 inverse(Vector2) const;
    Transformation inverse(Transformation) const;
    Params d(Vector2, int direction) const;
protected:
    void update_params(Matrix33 in_params);  /// Precalculate the transformation derivatives
};

Vector2 extract_point(Transformation::Params, unsigned);
#endif
