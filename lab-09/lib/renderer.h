#ifndef RENDERER_H
#define RENDERER_H

#include "math_3d.h"
#include "geometry.h"
#include <cmath>
#include <SFML/Graphics.hpp>
#include <sstream>
#include <fstream>
#include <iostream>
#include <functional>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Matrix4x4 createTranslationMatrix(double tx, double ty, double tz) {
    Matrix4x4 mat;
    mat.m[0][3] = tx;
    mat.m[1][3] = ty;
    mat.m[2][3] = tz;
    return mat;
}

Matrix4x4 createScaleMatrix(double sx, double sy, double sz) {
    Matrix4x4 mat;
    mat.m[0][0] = sx;
    mat.m[1][1] = sy;
    mat.m[2][2] = sz;
    return mat;
}

Matrix4x4 createRotationXMatrix(double angle) {
    double rad = angle * M_PI / 180.0;
    Matrix4x4 mat;
    mat.m[1][1] = cos(rad);
    mat.m[1][2] = sin(rad);
    mat.m[2][1] = -sin(rad);
    mat.m[2][2] = cos(rad);
    return mat;
}

Matrix4x4 createRotationYMatrix(double angle) {
    double rad = angle * M_PI / 180.0;
    Matrix4x4 mat;
    mat.m[0][0] = cos(rad);
    mat.m[0][2] = -sin(rad);
    mat.m[2][0] = sin(rad);
    mat.m[2][2] = cos(rad);
    return mat;
}

Matrix4x4 createRotationZMatrix(double angle) {
    double rad = angle * M_PI / 180.0;
    Matrix4x4 mat;
    mat.m[0][0] = cos(rad);
    mat.m[0][1] = sin(rad);
    mat.m[1][0] = -sin(rad);
    mat.m[1][1] = cos(rad);
    return mat;
}

Matrix4x4 createArbitraryRotationMatrix(const Point3D& p1, const Point3D& p2, double angle) {
    Point3D axis = p2 - p1;
    double len = sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);

    if (len == 0.0) {
        Matrix4x4 I;
        I.identity();
        return I;
    }

    double u = axis.x / len;
    double v = axis.y / len;
    double w = axis.z / len;
    
    double rad = angle * M_PI / 180.0;
    double cosA = cos(rad);
    double sinA = sin(rad);
    
    Matrix4x4 T = createTranslationMatrix(-p1.x, -p1.y, -p1.z);
    
    Matrix4x4 R;
    R.m[0][0] = u*u + (1 - u*u)*cosA;
    R.m[0][1] = u*v*(1 - cosA) - w*sinA;
    R.m[0][2] = u*w*(1 - cosA) + v*sinA;
    R.m[1][0] = u*v*(1 - cosA) + w*sinA;
    R.m[1][1] = v*v + (1 - v*v)*cosA;
    R.m[1][2] = v*w*(1 - cosA) - u*sinA;
    R.m[2][0] = u*w*(1 - cosA) - v*sinA;
    R.m[2][1] = v*w*(1 - cosA) + u*sinA;
    R.m[2][2] = w*w + (1 - w*w)*cosA;
    
    Matrix4x4 T_inv = createTranslationMatrix(p1.x, p1.y, p1.z);
    
    return T_inv * R * T;
}

Matrix4x4 createReflectionMatrix(char plane) {
    Matrix4x4 mat;
    switch (plane) {
        case 'X': mat.m[0][0] = -1; break;
        case 'Y': mat.m[1][1] = -1; break;
        case 'Z': mat.m[2][2] = -1; break;
    }
    return mat;
}

Matrix4x4 createPerspectiveMatrix(double fovY, double aspect, double zNear, double zFar) {
    Matrix4x4 mat;
    double f = 1.0 / tan(fovY * M_PI / 360.0);
    mat.m[0][0] = f / aspect;
    mat.m[1][1] = f;
    mat.m[2][2] = (zFar + zNear) / (zNear - zFar);
    mat.m[2][3] = (2.0 * zFar * zNear) / (zNear - zFar);
    mat.m[3][2] = -1.0;
    mat.m[3][3] = 0.0;
    return mat;
}

Matrix4x4 createAxonometricMatrix(double left, double right, double bottom, double top, double zNear, double zFar) {
    Matrix4x4 mat;
    mat.m[0][0] = 2.0 / (right - left);
    mat.m[1][1] = 2.0 / (top - bottom);
    mat.m[2][2] = -2.0 / (zFar - zNear);
    mat.m[0][3] = -(right + left) / (right - left);
    mat.m[1][3] = -(top + bottom) / (top - bottom);
    mat.m[2][3] = -(zFar + zNear) / (zFar - zNear);
    return mat;
}

// Функции создания многогранников
Polyhedron createHexahedron() {
    std::vector<Point3D> vertices = {
        Point3D(-0.7, -0.7, -0.7), Point3D(0.7, -0.7, -0.7), 
        Point3D(0.7, 0.7, -0.7), Point3D(-0.7, 0.7, -0.7),
        Point3D(-0.7, -0.7, 0.7), Point3D(0.7, -0.7, 0.7),
        Point3D(0.7, 0.7, 0.7), Point3D(-0.7, 0.7, 0.7)
    };
    
    std::vector<std::vector<int>> faces = {
        {3, 2, 1, 0}, // Задняя (было 0,1,2,3 - смотрела внутрь)
        {4, 5, 6, 7}, // Передняя
        {0, 1, 5, 4}, // Нижняя
        {2, 3, 7, 6}, // Верхняя
        {0, 4, 7, 3}, // Левая (было 0,3,7,4 - смотрела внутрь)
        {1, 2, 6, 5}  // Правая
    };
    
    std::vector<std::vector<Point3D>> texCoordsList = {
        {Point3D(0,0,0), Point3D(1,0,0), Point3D(1,1,0), Point3D(0,1,0)},
        {Point3D(0,0,0), Point3D(1,0,0), Point3D(1,1,0), Point3D(0,1,0)},
        {Point3D(0,0,0), Point3D(1,0,0), Point3D(1,1,0), Point3D(0,1,0)},
        {Point3D(0,0,0), Point3D(1,0,0), Point3D(1,1,0), Point3D(0,1,0)},
        {Point3D(0,0,0), Point3D(1,0,0), Point3D(1,1,0), Point3D(0,1,0)},
        {Point3D(0,0,0), Point3D(1,0,0), Point3D(1,1,0), Point3D(0,1,0)}
    };
    
    std::vector<Polygon> polygons;
    for (size_t i = 0; i < faces.size(); i++) {
        std::vector<Point3D> polygonPoints;
        for (int idx : faces[i]) {
            polygonPoints.push_back(vertices[idx]);
        }
        polygons.push_back(Polygon(polygonPoints, texCoordsList[i]));
    }
    
    return Polyhedron(polygons);
}

Polyhedron createIcosahedron() {
    double phi = (1.0 + sqrt(5.0)) / 2.0;
    double a = 1.0;
    double b = 1.0 / phi;
    
    std::vector<Point3D> vertices = {
        Point3D(0, b, -a), Point3D(b, a, 0), Point3D(-b, a, 0),
        Point3D(0, b, a), Point3D(0, -b, a), Point3D(-a, 0, b),
        Point3D(0, -b, -a), Point3D(a, 0, -b), Point3D(a, 0, b),
        Point3D(-a, 0, -b), Point3D(b, -a, 0), Point3D(-b, -a, 0)
    };
    
    // Масштабируем для лучшего отображения
    for (auto& v : vertices) {
        v = v * 0.5;
    }
    
    std::vector<std::vector<int>> faces = {
        {0, 1, 2}, {3, 2, 1}, {3, 4, 5}, {3, 8, 4},
        {0, 6, 7}, {0, 9, 6}, {4, 10, 11}, {6, 11, 10},
        {2, 5, 9}, {11, 9, 5}, {1, 7, 8}, {10, 8, 7},
        {3, 5, 2}, {3, 1, 8}, {0, 2, 9}, {0, 7, 1},
        {6, 9, 11}, {6, 10, 7}, {4, 11, 5}, {4, 8, 10}
    };
    
    std::vector<Polygon> polygons;
    for (const auto& face : faces) {
        std::vector<Point3D> polygonPoints;
        std::vector<Point3D> texPoints;
        for (int idx : face) {
            polygonPoints.push_back(vertices[idx]);
            // Простые текстурные координаты для демонстрации
            texPoints.push_back(Point3D((vertices[idx].x+1)/2, (vertices[idx].y+1)/2, 0));
        }
        polygons.push_back(Polygon(polygonPoints, texPoints));
    }
    
    return Polyhedron(polygons);
}

Polyhedron createTetrahedron() {
    std::vector<Point3D> vertices = {
        Point3D(0, 1, 0),
        Point3D(-0.866, -0.5, 0),
        Point3D(0.866, -0.5, 0),
        Point3D(0, 0, 1.414)
    };
    
    for (auto& v : vertices) {
        v = v * 0.5;
    }
    
    std::vector<std::vector<int>> faces = {
        {0, 1, 2},
        {0, 2, 3},
        {0, 3, 1},
        {1, 3, 2}
    };
    
    std::vector<Polygon> polygons;
    for (const auto& face : faces) {
        std::vector<Point3D> polygonPoints;
        std::vector<Point3D> texPoints;
        for (int idx : face) {
            polygonPoints.push_back(vertices[idx]);
            texPoints.push_back(Point3D((vertices[idx].x+1)/2, (vertices[idx].y+1)/2, 0));
        }
        polygons.push_back(Polygon(polygonPoints, texPoints));
    }
    
    return Polyhedron(polygons);
}

Polyhedron createOctahedron() {
    std::vector<Point3D> vertices = {
        Point3D(0, 1, 0),
        Point3D(1, 0, 0),
        Point3D(0, 0, 1),
        Point3D(-1, 0, 0),
        Point3D(0, 0, -1),
        Point3D(0, -1, 0)
    };
    
    for (auto& v : vertices) {
        v = v * 0.5;
    }
    
    std::vector<std::vector<int>> faces = {
        {0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 1},
        {5, 2, 1}, {5, 3, 2}, {5, 4, 3}, {5, 1, 4}
    };
    
    std::vector<Polygon> polygons;
    for (const auto& face : faces) {
        std::vector<Point3D> polygonPoints;
        std::vector<Point3D> texPoints;
        for (int idx : face) {
            polygonPoints.push_back(vertices[idx]);
            texPoints.push_back(Point3D((vertices[idx].x+1)/2, (vertices[idx].y+1)/2, 0));
        }
        polygons.push_back(Polygon(polygonPoints, texPoints));
    }
    
    return Polyhedron(polygons);
}

sf::Vector2f project(Point3D point, const Matrix4x4& mvp, int width, int height) {
    Point3D transformed = mvp.transform(point);

    if (transformed.w != 0) {
        transformed.x /= transformed.w;
        transformed.y /= transformed.w;
    }

    // Перевод из нормализованных координат (-1, 1) в экранные
    float screenX = (transformed.x + 1) * width / 2.0f;
    float screenY = (-transformed.y + 1) * height / 2.0f;
    
    return sf::Vector2f(screenX, screenY);
}

// lab 07

Polyhedron loadOBJ(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        return {};
    }

    std::vector<Point3D> vertices;
    std::vector<Point3D> texCoords;
    std::vector<Polygon> polygons;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            double x, y, z;
            iss >> x >> y >> z;
            vertices.emplace_back(x, y, z);
        } 
        else if (type == "vt") {
            double u, v;
            iss >> u >> v;
            texCoords.emplace_back(u, v, 0);
        }
        else if (type == "f") {
            std::vector<Point3D> face;
            std::vector<Point3D> faceTexCoords;
            std::string token;
            while (iss >> token) {
                std::stringstream ss(token);
                std::string indexStr, texIndexStr;
                std::getline(ss, indexStr, '/');
                std::getline(ss, texIndexStr, '/');
                
                int idx = std::stoi(indexStr);
                if (idx < 0) idx = vertices.size() + idx + 1;
                face.push_back(vertices[idx - 1]);
                
                if (!texIndexStr.empty()) {
                    int texIdx = std::stoi(texIndexStr);
                    if (texIdx < 0) texIdx = texCoords.size() + texIdx + 1;
                    if (texIdx > 0 && texIdx <= texCoords.size()) {
                        faceTexCoords.push_back(texCoords[texIdx - 1]);
                    }
                }
            }
            polygons.push_back(Polygon(face, faceTexCoords));
        }
    }

    std::cout << "Модель успешно загружена: " << polygons.size() << " полигонов." << std::endl;
    return Polyhedron(polygons);
}

void saveOBJ(const Polyhedron& polyhedron, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось создать файл " << filename << std::endl;
        return;
    }

    std::vector<Point3D> vertices;
    std::vector<Point3D> texCoords;
    for (const auto& polygon : polyhedron.polygons) {
        for (const auto& point : polygon.points) {
            vertices.push_back(point);
        }
        for (const auto& tex : polygon.texCoords) {
            texCoords.push_back(tex);
        }
    }

    for (const auto& v : vertices) {
        file << "v " << v.x << " " << v.y << " " << v.z << "\n";
    }
    
    for (const auto& vt : texCoords) {
        file << "vt " << vt.x << " " << vt.y << "\n";
    }

    int vertexIndex = 1;
    int texIndex = 1;
    for (const auto& polygon : polyhedron.polygons) {
        file << "f";
        for (size_t i = 0; i < polygon.points.size(); ++i) {
            file << " " << vertexIndex << "/" << texIndex;
            vertexIndex++;
            texIndex++;
        }
        file << "\n";
    }

    std::cout << "Модель сохранена в " << filename << std::endl;
}

Polyhedron generateSurfaceOfRevolution(
    const std::vector<Point3D>& profile,
    char axis,
    int segments)
{
    std::vector<Polygon> polygons;
    double angleStep = 2.0 * M_PI / segments;

    for (int i = 0; i < segments; ++i) {
        double theta1 = i * angleStep;
        double theta2 = (i + 1) * angleStep;

        std::vector<Point3D> ring1, ring2;
        std::vector<Point3D> texRing1, texRing2;
        
        for (size_t j = 0; j < profile.size(); ++j) {
            const auto& p = profile[j];
            double x = p.x, y = p.y, z = p.z;
            Point3D p1, p2;

            if (axis == 'y' || axis == 'Y') {
                p1 = Point3D(x * cos(theta1), y, x * sin(theta1));
                p2 = Point3D(x * cos(theta2), y, x * sin(theta2));
            } else if (axis == 'x' || axis == 'X') {
                p1 = Point3D(p.z * sin(theta1), p.y * cos(theta1), p.z * cos(theta1));
                p2 = Point3D(p.z * sin(theta2), p.y * cos(theta2), p.z * cos(theta2));
            } else { // ось Z
                p1 = Point3D(x * cos(theta1) - y * sin(theta1), x * sin(theta1) + y * cos(theta1), z);
                p2 = Point3D(x * cos(theta2) - y * sin(theta2), x * sin(theta2) + y * cos(theta2), z);
            }

            ring1.push_back(p1);
            ring2.push_back(p2);
            
            float u1 = (float)i / segments;
            float u2 = (float)(i+1) / segments;
            float v = (float)j / (profile.size() - 1);
            texRing1.push_back(Point3D(u1, v, 0));
            texRing2.push_back(Point3D(u2, v, 0));
        }

        for (size_t j = 0; j < profile.size() - 1; ++j) {
            polygons.push_back(Polygon({
                ring1[j], ring1[j + 1], ring2[j + 1], ring2[j]
            }, {
                texRing1[j], texRing1[j + 1], texRing2[j + 1], texRing2[j]
            }));
        }
    }

    return Polyhedron(polygons);
}

Polyhedron generateFunctionSurface(
    std::function<double(double, double)> func,
    double x0, double x1,
    double y0, double y1,
    int steps)
{
    std::vector<Point3D> points;
    std::vector<Polygon> polygons;

    double dx = (x1 - x0) / steps;
    double dy = (y1 - y0) / steps;

    for (int i = 0; i <= steps; ++i) {
        double x = x0 + i * dx;
        for (int j = 0; j <= steps; ++j) {
            double y = y0 + j * dy;
            double z = func(x, y);
            points.push_back(Point3D(x, y, z));
        }
    }

    for (int i = 0; i < steps; ++i) {
        for (int j = 0; j < steps; ++j) {
            int idx = i * (steps + 1) + j;
            float u1 = (float)i / steps;
            float u2 = (float)(i+1) / steps;
            float v1 = (float)j / steps;
            float v2 = (float)(j+1) / steps;
            
            polygons.push_back(Polygon({
                points[idx],
                points[idx + 1],
                points[idx + steps + 2],
                points[idx + steps + 1]
            }, {
                Point3D(u1, v1, 0),
                Point3D(u2, v1, 0),
                Point3D(u2, v2, 0),
                Point3D(u1, v2, 0)
            }));
        }
    }

    return Polyhedron(polygons);
}

#endif