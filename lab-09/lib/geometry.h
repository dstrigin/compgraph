#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "math_3d.h"
#include <vector>
#include <map>
#include <SFML/Graphics.hpp>

class Polygon {
public:
    std::vector<Point3D> points;
    
    Polygon(const std::vector<Point3D>& points = {}) : points(points) {}

    void transform(const Matrix4x4& matrix);
    Point3D getNormal() const;
};

class Polyhedron {
public:
    std::vector<Polygon> polygons;
    
    Polyhedron(const std::vector<Polygon>& polygons = {}) : polygons(polygons) {}
    
    void transform(const Matrix4x4& matrix);
    Point3D getCenter() const;
};

struct Light {
    Point3D position;
    sf::Color color;
    float intensity; // 0.0 - 1.0
};

// Функция для сглаживания нормалей (для Гуро и Фонга)
// Возвращает мапу [Вершина] -> Усредненная нормаль
std::map<Point3D, Point3D> calculateSmoothNormals(const Polyhedron& poly);

#endif
