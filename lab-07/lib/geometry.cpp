#include "geometry.h"

void Polygon::transform(const Matrix4x4& matrix) {
    for (auto& point : points) {
        point = matrix.transform(point);
    }
}

void Polyhedron::transform(const Matrix4x4& matrix) {
    for (auto& polygon : polygons) {
        polygon.transform(matrix);
    }
}

Point3D Polyhedron::getCenter() const {
    Point3D center;
    int count = 0;
    for (const auto& polygon : polygons) {
        for (const auto& point : polygon.points) {
            center = center + point;
            count++;
        }
    }
    if (count > 0) {
        center = center * (1.0 / count);
    }
    return center;
}