#include "geometry.h"

void Polygon::transform(const Matrix4x4& matrix) {
    for (auto& point : points) {
        point = matrix.transform(point);
    }
    // Текстурные координаты не трансформируются
}

Point3D Polygon::getNormal() const {
    if (points.size() < 3) return Point3D(0,0,0,0);
    Point3D a = points[1] - points[0];
    Point3D b = points[2] - points[0];
    return a.cross(b).normalize();
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

std::map<Point3D, Point3D> calculateSmoothNormals(const Polyhedron& poly) {
    std::map<Point3D, Point3D> normalsMap;
    
    for (const auto& polygon : poly.polygons) {
        Point3D faceNormal = polygon.getNormal();
        for (const auto& point : polygon.points) {
            normalsMap[point] = normalsMap[point] + faceNormal;
        }
    }
    
    for (auto& pair : normalsMap) {
        pair.second = pair.second.normalize();
    }
    
    return normalsMap;
}