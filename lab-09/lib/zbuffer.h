#ifndef ZBUFFER_H
#define ZBUFFER_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <limits>
#include <algorithm>
#include "math_3d.h"
#include "geometry.h"

class ZBuffer {
private:
    int width, height;
    std::vector<float> zBuffer;
    sf::Image frameBuffer;
    Texture* currentTexture;
    
public:
    ZBuffer(int w, int h) : width(w), height(h), currentTexture(nullptr) {
        zBuffer.resize(width * height);
        frameBuffer.create(width, height, sf::Color::Black);
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
    void setTexture(Texture* texture) {
        currentTexture = texture;
    }
    
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
    void rasterizeTriangleWithTexture(const Point3D& p1, const Point3D& p2, const Point3D& p3,
                                     const Point3D& t1, const Point3D& t2, const Point3D& t3,
                                     const Matrix4x4& mvp, bool backfaceCulling = true) {
        
        if (!currentTexture) return;
        
        Point3D v1 = mvp.transform(p1);
        Point3D v2 = mvp.transform(p2);
        Point3D v3 = mvp.transform(p3);
        
        if (v1.w != 0) { v1.x /= v1.w; v1.y /= v1.w; v1.z /= v1.w; }
        if (v2.w != 0) { v2.x /= v2.w; v2.y /= v2.w; v2.z /= v2.w; }
        if (v3.w != 0) { v3.x /= v3.w; v3.y /= v3.w; v3.z /= v3.w; }
        
        if (backfaceCulling) {
            Point3D edge1 = v2 - v1;
            Point3D edge2 = v3 - v1;
            Point3D normal = edge1.cross(edge2);
            if (normal.z <= 0) return;
        }
        
        int x1 = (int)((v1.x + 1.0) * width / 2.0);
        int y1 = (int)((-v1.y + 1.0) * height / 2.0);
        float z1 = v1.z;
        
        int x2 = (int)((v2.x + 1.0) * width / 2.0);
        int y2 = (int)((-v2.y + 1.0) * height / 2.0);
        float z2 = v2.z;
        
        int x3 = (int)((v3.x + 1.0) * width / 2.0);
        int y3 = (int)((-v3.y + 1.0) * height / 2.0);
        float z3 = v3.z;
        
        int minX = std::max(0, std::min({x1, x2, x3}));
        int maxX = std::min(width - 1, std::max({x1, x2, x3}));
        int minY = std::max(0, std::min({y1, y2, y3}));
        int maxY = std::min(height - 1, std::max({y1, y2, y3}));
        
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                float denom = (float)((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
                if (denom == 0) continue;
                
                float w1 = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) / denom;
                float w2 = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) / denom;
                float w3 = 1.0f - w1 - w2;
                
                if (w1 >= 0 && w2 >= 0 && w3 >= 0) {
                    float z = w1 * z1 + w2 * z2 + w3 * z3;
                    
                    int idx = y * width + x;
                    
                    if (z < zBuffer[idx]) {
                        zBuffer[idx] = z;
                        
                        float u = w1 * t1.x + w2 * t2.x + w3 * t3.x;
                        float v = w1 * t1.y + w2 * t2.y + w3 * t3.y;
                        
                        sf::Color texColor = currentTexture->getColor(u, v);
                        frameBuffer.setPixel(x, y, texColor);
                    }
                }
            }
        }
    }
    
    void rasterizePolygonWithTexture(const Polygon& polygon, 
                                    const Matrix4x4& mvp, 
                                    bool backfaceCulling = true) {
        if (polygon.points.size() < 3) return;
        
        for (size_t i = 1; i < polygon.points.size() - 1; i++) {
            rasterizeTriangleWithTexture(
                polygon.points[0],
                polygon.points[i],
                polygon.points[i + 1],
                polygon.texCoords[0],
                polygon.texCoords[i],
                polygon.texCoords[i + 1],
                mvp, backfaceCulling
            );
        }
    }

    // шейдинг Гуро
    
    void rasterizeTriangleGouraud(
        const Point3D& p1, const Point3D& p2, const Point3D& p3,
        const Point3D& n1, const Point3D& n2, const Point3D& n3,
        const sf::Color& color, 
        const Matrix4x4& mvp, const Matrix4x4& model, 
        const Light& light) 
    {
        // Вспомогательная лямбда для модели Ламберта (Diff = max(0, N*L))
        auto calculateLighting = [&](const Point3D& vertexPos, const Point3D& normal) -> float {
            Point3D worldPos = model.transform(vertexPos); // Позиция в мире
            Point3D worldNormal = model.transform(Point3D(normal.x, normal.y, normal.z, 0)).normalize();
            
            Point3D lightDir = (light.position - worldPos).normalize();
            double diff = std::max(0.0, worldNormal.dot(lightDir));
            return (float)diff;
        };

        float i1 = calculateLighting(p1, n1);
        float i2 = calculateLighting(p2, n2);
        float i3 = calculateLighting(p3, n3);

        Point3D v1 = mvp.transform(p1);
        Point3D v2 = mvp.transform(p2);
        Point3D v3 = mvp.transform(p3);
        
        if (v1.w != 0) { v1.x /= v1.w; v1.y /= v1.w; v1.z /= v1.w; }
        if (v2.w != 0) { v2.x /= v2.w; v2.y /= v2.w; v2.z /= v2.w; }
        if (v3.w != 0) { v3.x /= v3.w; v3.y /= v3.w; v3.z /= v3.w; }

        // Backface culling
        if (((v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x)) <= 0) return;

        // Экранные координаты
        int x1 = (int)((v1.x + 1.0) * width / 2.0);
        int y1 = (int)((-v1.y + 1.0) * height / 2.0);
        int x2 = (int)((v2.x + 1.0) * width / 2.0);
        int y2 = (int)((-v2.y + 1.0) * height / 2.0);
        int x3 = (int)((v3.x + 1.0) * width / 2.0);
        int y3 = (int)((-v3.y + 1.0) * height / 2.0);

        int minX = std::max(0, std::min({x1, x2, x3}));
        int maxX = std::min(width - 1, std::max({x1, x2, x3}));
        int minY = std::max(0, std::min({y1, y2, y3}));
        int maxY = std::min(height - 1, std::max({y1, y2, y3}));

        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                float denom = (float)((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
                if (denom == 0) continue;
                
                float w1 = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) / denom;
                float w2 = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) / denom;
                float w3 = 1.0f - w1 - w2;

                if (w1 >= 0 && w2 >= 0 && w3 >= 0) {
                    float z = w1 * v1.z + w2 * v2.z + w3 * v3.z;
                    int idx = y * width + x;

                    if (z < zBuffer[idx]) {
                        zBuffer[idx] = z;
                        
                        // Интерполяция интенсивности (Гуро)
                        float pixelIntensity = w1 * i1 + w2 * i2 + w3 * i3;
                        
                        // Применение интенсивности к цвету
                        sf::Uint8 r = (sf::Uint8)std::min(255.0f, color.r * pixelIntensity * light.intensity + 10); // +10 ambient
                        sf::Uint8 g = (sf::Uint8)std::min(255.0f, color.g * pixelIntensity * light.intensity + 10);
                        sf::Uint8 b = (sf::Uint8)std::min(255.0f, color.b * pixelIntensity * light.intensity + 10);
                        
                        frameBuffer.setPixel(x, y, sf::Color(r, g, b));
                    }
                }
            }
        }
    }

    void rasterizeTrianglePhongToon(
        const Point3D& p1, const Point3D& p2, const Point3D& p3,
        const Point3D& n1, const Point3D& n2, const Point3D& n3,
        const sf::Color& color, 
        const Matrix4x4& mvp, const Matrix4x4& model, 
        const Light& light) 
    {
        Point3D v1 = mvp.transform(p1);
        Point3D v2 = mvp.transform(p2);
        Point3D v3 = mvp.transform(p3);
        
        Point3D wP1 = model.transform(p1);
        Point3D wP2 = model.transform(p2);
        Point3D wP3 = model.transform(p3);

        Point3D wN1 = model.transform(Point3D(n1.x, n1.y, n1.z, 0)).normalize();
        Point3D wN2 = model.transform(Point3D(n2.x, n2.y, n2.z, 0)).normalize();
        Point3D wN3 = model.transform(Point3D(n3.x, n3.y, n3.z, 0)).normalize();

        if (v1.w != 0) { v1.x /= v1.w; v1.y /= v1.w; v1.z /= v1.w; }
        if (v2.w != 0) { v2.x /= v2.w; v2.y /= v2.w; v2.z /= v2.w; }
        if (v3.w != 0) { v3.x /= v3.w; v3.y /= v3.w; v3.z /= v3.w; }

        // Backface culling
        if (((v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x)) <= 0) return;

        int x1 = (int)((v1.x + 1.0) * width / 2.0);
        int y1 = (int)((-v1.y + 1.0) * height / 2.0);
        int x2 = (int)((v2.x + 1.0) * width / 2.0);
        int y2 = (int)((-v2.y + 1.0) * height / 2.0);
        int x3 = (int)((v3.x + 1.0) * width / 2.0);
        int y3 = (int)((-v3.y + 1.0) * height / 2.0);

        int minX = std::max(0, std::min({x1, x2, x3}));
        int maxX = std::min(width - 1, std::max({x1, x2, x3}));
        int minY = std::max(0, std::min({y1, y2, y3}));
        int maxY = std::min(height - 1, std::max({y1, y2, y3}));

        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                // Барицентрические координаты
                float denom = (float)((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
                if (denom == 0) continue;
                
                float w1 = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) / denom;
                float w2 = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) / denom;
                float w3 = 1.0f - w1 - w2;

                if (w1 >= 0 && w2 >= 0 && w3 >= 0) {
                    float z = w1 * v1.z + w2 * v2.z + w3 * v3.z;
                    int idx = y * width + x;

                    if (z < zBuffer[idx]) {
                        zBuffer[idx] = z;

                        Point3D pixelWorldPos = wP1 * w1 + wP2 * w2 + wP3 * w3;
                        Point3D pixelNormal = wN1 * w1 + wN2 * w2 + wN3 * w3;
                        pixelNormal = pixelNormal.normalize(); // Важно: повторная нормализация

                        Point3D lightDir = (light.position - pixelWorldPos).normalize();
                        
                        float diff = 0.2f + std::max(0.0, pixelNormal.dot(lightDir));
                        //float intensityFactor = diff;
                         float intensityFactor = 1.0f;

                        if (diff < 0.4f) {
                            intensityFactor = diff * 0.3f; // Тень
                        } else if (diff < 0.7f) {
                            intensityFactor = diff * 1.0f; // Основной цвет
                        } else {
                            intensityFactor = diff * 1.3f; // Блик (Specular имитация)
                        }
                        
                        // Применяем результат
                        sf::Uint8 r = (sf::Uint8)std::min(255.0f, color.r * intensityFactor * light.intensity);
                        sf::Uint8 g = (sf::Uint8)std::min(255.0f, color.g * intensityFactor * light.intensity);
                        sf::Uint8 b = (sf::Uint8)std::min(255.0f, color.b * intensityFactor * light.intensity);

                        frameBuffer.setPixel(x, y, sf::Color(r, g, b));
                    }
                }
            }
        }
    }
    
    const sf::Image& getFrameBuffer() const {
        return frameBuffer;
    }
    
    sf::Image getZBufferVisualization() const {
        sf::Image zbufferImg;
        zbufferImg.create(width, height);
        
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
                    int intensity = (int)(255.0f * (z - minZ) / range);
                    zbufferImg.setPixel(x, y, sf::Color(intensity, intensity, intensity));
                }
            }
        }
        
        return zbufferImg;
    }
};

#endif