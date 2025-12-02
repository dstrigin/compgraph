#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "math_3d.h"
#include <vector>
#include <map>
#include <SFML/Graphics.hpp>

class Polygon {
public:
    std::vector<Point3D> points;
    std::vector<Point3D> texCoords; // Текстурные координаты
    
    Polygon(const std::vector<Point3D>& points = {}, 
            const std::vector<Point3D>& texCoords = {}) 
        : points(points), texCoords(texCoords) {}

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

struct Texture {
    sf::Image image;
    int width, height;
    
    Texture() : width(0), height(0) {}
    
    bool loadFromFile(const std::string& filename) {
        if (image.loadFromFile(filename)) {
            width = image.getSize().x;
            height = image.getSize().y;
            return true;
        }
        return false;
    }
    
    sf::Color getColor(float u, float v) const {
        if (width == 0 || height == 0) return sf::Color::White;
        
        u = u - floor(u);
        v = v - floor(v);
        if (u < 0) u += 1.0f;
        if (v < 0) v += 1.0f;
        
        int x = (int)(u * width) % width;
        int y = (int)(v * height) % height;
        
        return image.getPixel(x, y);
    }
};

// Функция для сглаживания нормалей (для Гуро и Фонга)
// Возвращает мапу [Вершина] -> Усредненная нормаль
std::map<Point3D, Point3D> calculateSmoothNormals(const Polyhedron& poly);

#endif