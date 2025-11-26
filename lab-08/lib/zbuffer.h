#ifndef ZBUFFER_H
#define ZBUFFER_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <limits>
#include "math_3d.h"
#include "geometry.h"

class ZBuffer {
private:
    int width, height;
    std::vector<float> zBuffer;
    sf::Image frameBuffer;
    
public:
    ZBuffer(int w, int h) : width(w), height(h) {
        zBuffer.resize(width * height);
        frameBuffer.create(width, height);
        clear();
    }
    
    void clear() {
        // Заполнить буфер кадра фоновым значением
        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                frameBuffer.setPixel(i, j, sf::Color::Black);
            }
        }
        
        // Заполнить z-буфер максимальным значением z
        std::fill(zBuffer.begin(), zBuffer.end(), std::numeric_limits<float>::max());
    }
    
    // Растеризация треугольника с использованием z-буфера
    void rasterizeTriangle(const Point3D& p1, const Point3D& p2, const Point3D& p3, 
                          const sf::Color& color, const Matrix4x4& mvp, bool backfaceCulling = true) {
        
        // Преобразование вершин
        Point3D v1 = mvp.transform(p1);
        Point3D v2 = mvp.transform(p2);
        Point3D v3 = mvp.transform(p3);
        
        // Перспективное деление
        if (v1.w != 0) { v1.x /= v1.w; v1.y /= v1.w; v1.z /= v1.w; }
        if (v2.w != 0) { v2.x /= v2.w; v2.y /= v2.w; v2.z /= v2.w; }
        if (v3.w != 0) { v3.x /= v3.w; v3.y /= v3.w; v3.z /= v3.w; }
        
        // Отсечение невидимых граней (backface culling)
        if (backfaceCulling) {
            Point3D edge1 = v2 - v1;
            Point3D edge2 = v3 - v1;
            Point3D normal = edge1.cross(edge2);
            
            // Если нормаль направлена от камеры (z < 0), пропускаем
            if (normal.z <= 0) return;
        }
        
        // Преобразование в экранные координаты
        int x1 = (int)((v1.x + 1.0) * width / 2.0);
        int y1 = (int)((-v1.y + 1.0) * height / 2.0);
        float z1 = v1.z;
        
        int x2 = (int)((v2.x + 1.0) * width / 2.0);
        int y2 = (int)((-v2.y + 1.0) * height / 2.0);
        float z2 = v2.z;
        
        int x3 = (int)((v3.x + 1.0) * width / 2.0);
        int y3 = (int)((-v3.y + 1.0) * height / 2.0);
        float z3 = v3.z;
        
        // Находим ограничивающий прямоугольник
        int minX = std::max(0, std::min({x1, x2, x3}));
        int maxX = std::min(width - 1, std::max({x1, x2, x3}));
        int minY = std::max(0, std::min({y1, y2, y3}));
        int maxY = std::min(height - 1, std::max({y1, y2, y3}));
        
        // Растеризация треугольника
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                // Барицентрические координаты
                float w1 = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) / 
                          (float)((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
                float w2 = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) / 
                          (float)((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
                float w3 = 1.0f - w1 - w2;
                
                // Проверка, находится ли точка внутри треугольника
                if (w1 >= 0 && w2 >= 0 && w3 >= 0) {
                    // Интерполяция глубины
                    float z = w1 * z1 + w2 * z2 + w3 * z3;
                    
                    int idx = y * width + x;
                    
                    // Сравнить глубину z(x, y) со значением в z-буфере
                    if (z < zBuffer[idx]) {
                        // Если z(x, y) < Z_буфер(x, y), обновляем оба буфера
                        zBuffer[idx] = z;
                        frameBuffer.setPixel(x, y, color);
                    }
                }
            }
        }
    }
    
    // Растеризация полигона (разбиваем на треугольники)
    void rasterizePolygon(const Polygon& polygon, const sf::Color& color, 
                         const Matrix4x4& mvp, bool backfaceCulling = true) {
        if (polygon.points.size() < 3) return;
        
        // Треугольная веерная триангуляция от первой вершины
        for (size_t i = 1; i < polygon.points.size() - 1; i++) {
            rasterizeTriangle(
                polygon.points[0],
                polygon.points[i],
                polygon.points[i + 1],
                color, mvp, backfaceCulling
            );
        }
    }
    
    const sf::Image& getFrameBuffer() const {
        return frameBuffer;
    }
    
    // Визуализация z-буфера для отладки
    sf::Image getZBufferVisualization() const {
        sf::Image zbufferImg;
        zbufferImg.create(width, height);
        
        // Находим min и max значения для нормализации
        float minZ = std::numeric_limits<float>::max();
        float maxZ = -std::numeric_limits<float>::max();
        
        for (float z : zBuffer) {
            if (z != std::numeric_limits<float>::max()) {
                minZ = std::min(minZ, z);
                maxZ = std::max(maxZ, z);
            }
        }
        
        float range = maxZ - minZ;
        if (range == 0) range = 1.0f;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float z = zBuffer[y * width + x];
                
                if (z == std::numeric_limits<float>::max()) {
                    zbufferImg.setPixel(x, y, sf::Color::Black);
                } else {
                    // Нормализация в диапазон [0, 255]
                    int intensity = (int)(255.0f * (z - minZ) / range);
                    zbufferImg.setPixel(x, y, sf::Color(intensity, intensity, intensity));
                }
            }
        }
        
        return zbufferImg;
    }
};

#endif
