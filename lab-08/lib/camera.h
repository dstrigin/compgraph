#ifndef CAMERA_H
#define CAMERA_H

#include "math_3d.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Camera {
public:
    Point3D position;
    Point3D target;
    Point3D up;
    float radius;
    float angleH; // горизонтальный угол (вокруг Y)
    float angleV; // вертикальный угол
    
    Camera(Point3D pos = Point3D(0, 0, 3), Point3D tgt = Point3D(0, 0, 0))
        : position(pos), target(tgt), up(0, 1, 0), angleH(0), angleV(20) {
        radius = sqrt(pow(position.x - target.x, 2) + 
                     pow(position.y - target.y, 2) + 
                     pow(position.z - target.z, 2));
    }
    
    void rotateAroundTarget(float deltaH, float deltaV) {
        angleH += deltaH;
        angleV += deltaV;
        
        // Ограничиваем вертикальный угол
        if (angleV > 89.0f) angleV = 89.0f;
        if (angleV < -89.0f) angleV = -89.0f;
        
        updatePosition();
    }
    
    void updatePosition() {
        float angleH_rad = angleH * M_PI / 180.0f;
        float angleV_rad = angleV * M_PI / 180.0f;
        
        position.x = target.x + radius * cos(angleV_rad) * sin(angleH_rad);
        position.y = target.y + radius * sin(angleV_rad);
        position.z = target.z + radius * cos(angleV_rad) * cos(angleH_rad);
    }
    
    Matrix4x4 getViewMatrix() {
        // Вычисляем направление взгляда (от камеры к цели)
        Point3D forward = (target - position).normalize();
        
        // Вычисляем вправо (right vector)
        Point3D right = forward.cross(up).normalize();
        
        // Пересчитываем up
        Point3D newUp = right.cross(forward).normalize();
        
        // Создаем матрицу вида (lookAt)
        Matrix4x4 viewMatrix;
        viewMatrix.m[0][0] = right.x;
        viewMatrix.m[0][1] = right.y;
        viewMatrix.m[0][2] = right.z;
        viewMatrix.m[0][3] = -right.dot(position);
        
        viewMatrix.m[1][0] = newUp.x;
        viewMatrix.m[1][1] = newUp.y;
        viewMatrix.m[1][2] = newUp.z;
        viewMatrix.m[1][3] = -newUp.dot(position);
        
        viewMatrix.m[2][0] = -forward.x;
        viewMatrix.m[2][1] = -forward.y;
        viewMatrix.m[2][2] = -forward.z;
        viewMatrix.m[2][3] = forward.dot(position);
        
        viewMatrix.m[3][0] = 0;
        viewMatrix.m[3][1] = 0;
        viewMatrix.m[3][2] = 0;
        viewMatrix.m[3][3] = 1;
        
        return viewMatrix;
    }
};

#endif
