#ifndef MATH_3D_H
#define MATH_3D_H

class Point3D {
public:
    double x, y, z, w;
    
    Point3D(double x = 0, double y = 0, double z = 0, double w = 1) 
        : x(x), y(y), z(z), w(w) {}
    
    Point3D operator+(const Point3D& other) const;
    Point3D operator-(const Point3D& other) const;
    Point3D operator*(double scalar) const;
};

class Matrix4x4 {
public:
    double m[4][4];
    
    Matrix4x4() {
        identity();
    }
    
    void identity();
    
    Matrix4x4 operator*(const Matrix4x4& other) const;
    Point3D transform(const Point3D& point) const;
};

#endif