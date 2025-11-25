#include "math_3d.h"

Point3D Point3D::operator+(const Point3D& other) const {
    return Point3D(x + other.x, y + other.y, z + other.z);
}

Point3D Point3D::operator-(const Point3D& other) const {
    return Point3D(x - other.x, y - other.y, z - other.z);
}

Point3D Point3D::operator*(double scalar) const {
    return Point3D(x * scalar, y * scalar, z * scalar);
}

double Point3D::dot(const Point3D& other) const {
    return x * other.x + y * other.y + z * other.z;
}

Point3D Point3D::cross(const Point3D& other) const {
    return Point3D(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x,
        0 // w для вектора направления обычно 0, но здесь оставим 0 для безопасности
    );
}

double Point3D::length() const {
    return std::sqrt(x*x + y*y + z*z);
}

Point3D Point3D::normalize() const {
    double len = length();
    if (len == 0) return Point3D(0, 0, 0, 0);
    return Point3D(x / len, y / len, z / len, 0);
}

void Matrix4x4::identity() {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            this->m[i][j] = (i == j) ? 1.0 : 0.0;
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& other) const {
    Matrix4x4 result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i][j] = 0;
            for (int k = 0; k < 4; k++) {
                result.m[i][j] += this->m[i][k] * other.m[k][j];
            }
        }
    }
    return result;
}

Point3D Matrix4x4::transform(const Point3D& point) const {
    double x = m[0][0] * point.x + m[0][1] * point.y + m[0][2] * point.z + m[0][3] * point.w;
    double y = m[1][0] * point.x + m[1][1] * point.y + m[1][2] * point.z + m[1][3] * point.w;
    double z = m[2][0] * point.x + m[2][1] * point.y + m[2][2] * point.z + m[2][3] * point.w;
    double w = m[3][0] * point.x + m[3][1] * point.y + m[3][2] * point.z + m[3][3] * point.w;
    
    if (w != 0 && w != 1) {
        x /= w; y /= w; z /= w;
        w = 1.0;
    }
    return Point3D(x, y, z, w);
}
