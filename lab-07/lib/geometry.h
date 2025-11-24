#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "math_3d.h"
#include <vector>

class Polygon {
public:
    std::vector<Point3D> points;
    
    Polygon(const std::vector<Point3D>& points = {}) : points(points) {}

    void transform(const Matrix4x4& matrix);
};

class Polyhedron {
public:
    std::vector<Polygon> polygons;
    
    Polyhedron(const std::vector<Polygon>& polygons = {}) : polygons(polygons) {}
    
    void transform(const Matrix4x4& matrix);
    Point3D getCenter() const;
};

#endif
