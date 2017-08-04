#error This file is only for documenation purposes.

struct Transformation
{
    using Params = MatrixMN;
    
    /// Region of interest
    Region region;
    
    const StaticParamsType static_params;
    InternalParamsType params;
    
    Transformation();
    
    /** Initialize an identity transformation
     */
    Transformation(Region region);
    
    /** Copy constructor
     */
    Transformation(Region, decltype(params), decltype(static_params));
    
    /** Update params so as to mimic another transformation
     */
    Transformation& operator = (const Transformation&);
    
    /** Increase params by a differential amount
     */
    Transformation operator + (Params) const;
    Transformation& operator += (Params);

    /** Modify a vertex of a cage-based transformation
     * (only available in barycentric and perspective)
     */
    Transformation& increment(Vector2, int);
    
    /** Transform from reference to view space
	 */
    Vector2 operator () (Vector2) const;
    Vector2 operator () (Pixel p) const { return (*this)(to_vector(p)); }
	
	/** Transform a bounding box from reference to view space
	 */
    Region operator () (Region) const;
    
    /** Estimate the scale factor
     */
    float scale(Vector2) const;
    Vector2 operator - (const Transformation&) const;

    /** Transformation from view to reference space
     */
    Transformation inverse() const;
    Vector2 inverse(Vector2) const { return inverse()(v); }

    /** Derivative of view space wrt. params
     * @param direction 0 for x, 1 for y
     */
    Params d(Vector2, int direction) const;
    
    /** Return the extremal points of the region
     */
    std::array<Vector2, N> vertices() const;
};
