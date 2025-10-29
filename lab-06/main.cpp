#include <GL/glut.h>
#include <GL/gl.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Point3D {
public:
    double x, y, z, w;
    
    Point3D(double x = 0, double y = 0, double z = 0, double w = 1) 
        : x(x), y(y), z(z), w(w) {}
    
    // Операторы для работы с точками
    Point3D operator+(const Point3D& other) const {
        return Point3D(x + other.x, y + other.y, z + other.z);
    }
    
    Point3D operator-(const Point3D& other) const {
        return Point3D(x - other.x, y - other.y, z - other.z);
    }
    
    Point3D operator*(double scalar) const {
        return Point3D(x * scalar, y * scalar, z * scalar);
    }
};

class Matrix4x4 {
public:
    double m[4][4];
    
    Matrix4x4() {
        identity();
    }
    
    void identity() {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                m[i][j] = (i == j) ? 1.0 : 0.0;
    }
    
    Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i][j] = 0;
                for (int k = 0; k < 4; k++) {
                    result.m[i][j] += m[i][k] * other.m[k][j];
                }
            }
        }
        return result;
    }
    
    Point3D transform(const Point3D& point) const {
        double x = m[0][0] * point.x + m[0][1] * point.y + m[0][2] * point.z + m[0][3] * point.w;
        double y = m[1][0] * point.x + m[1][1] * point.y + m[1][2] * point.z + m[1][3] * point.w;
        double z = m[2][0] * point.x + m[2][1] * point.y + m[2][2] * point.z + m[2][3] * point.w;
        double w = m[3][0] * point.x + m[3][1] * point.y + m[3][2] * point.z + m[3][3] * point.w;
        
        if (w != 0 && w != 1) {
            x /= w; y /= w; z /= w;
        }
        return Point3D(x, y, z, w);
    }
};

class Polygon {
public:
    std::vector<Point3D> points;
    
    Polygon(const std::vector<Point3D>& points = {}) : points(points) {}
    
    void transform(const Matrix4x4& matrix) {
        for (auto& point : points) {
            point = matrix.transform(point);
        }
    }
};

class Polyhedron {
public:
    std::vector<Polygon> polygons;
    
    Polyhedron(const std::vector<Polygon>& polygons = {}) : polygons(polygons) {}
    
    void transform(const Matrix4x4& matrix) {
        for (auto& polygon : polygons) {
            polygon.transform(matrix);
        }
    }
    
    Point3D getCenter() const {
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
};

// Глобальные переменные
Polyhedron currentPolyhedron;
float rotationX = 0.0f;
float rotationY = 0.0f;
bool perspectiveProjection = true;
Matrix4x4 currentTransformation;
std::string currentMode = "VIEW";

// Матричные преобразования
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
    mat.m[1][2] = -sin(rad);
    mat.m[2][1] = sin(rad);
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
    mat.m[0][1] = -sin(rad);
    mat.m[1][0] = sin(rad);
    mat.m[1][1] = cos(rad);
    return mat;
}

Matrix4x4 createArbitraryRotationMatrix(const Point3D& p1, const Point3D& p2, double angle) {
    Point3D axis = p2 - p1;
    double len = sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
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
        case 'X': mat.m[0][0] = -1; break; // YZ plane
        case 'Y': mat.m[1][1] = -1; break; // XZ plane
        case 'Z': mat.m[2][2] = -1; break; // XY plane
    }
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
        {0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4},
        {2, 3, 7, 6}, {0, 3, 7, 4}, {1, 2, 6, 5}
    };
    
    std::vector<Polygon> polygons;
    for (const auto& face : faces) {
        std::vector<Point3D> polygonPoints;
        for (int idx : face) {
            polygonPoints.push_back(vertices[idx]);
        }
        polygons.push_back(Polygon(polygonPoints));
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
        for (int idx : face) {
            polygonPoints.push_back(vertices[idx]);
        }
        polygons.push_back(Polygon(polygonPoints));
    }
    
    return Polyhedron(polygons);
}

void drawPolyhedron(const Polyhedron& poly) {
    glLineWidth(2.0f);
    
    GLfloat colors[][3] = {
        {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f},
        {0.5f, 0.5f, 1.0f}, {1.0f, 0.5f, 0.0f}, {0.5f, 1.0f, 0.5f}
    };
    
    for (size_t i = 0; i < poly.polygons.size(); i++) {
        const Polygon& polygon = poly.polygons[i];
        if (polygon.points.empty()) continue;
        
        glColor3fv(colors[i % 9]);
        
        glBegin(GL_LINE_LOOP);
        for (const Point3D& point : polygon.points) {
            glVertex3f(point.x, point.y, point.z);
        }
        glEnd();
        
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        for (const Point3D& point : polygon.points) {
            glVertex3f(point.x, point.y, point.z);
        }
        glEnd();
    }
}

void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    if (perspectiveProjection) {
        gluLookAt(3.0, 3.0, 3.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    } else {
        // Аксонометрическая проекция
        glRotatef(35.264, 1.0, 0.0, 0.0); // Изометрическая
        glRotatef(45.0, 0.0, 1.0, 0.0);
    }
    
    glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f);
    
    // Рисуем оси координат
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(2.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 2.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 2.0f);
    glEnd();
    
    // Применяем текущее преобразование
    Polyhedron transformedPoly = currentPolyhedron;
    transformedPoly.transform(currentTransformation);
    drawPolyhedron(transformedPoly);
    
    // Отладочная информация
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glColor3f(1.0, 1.0, 1.0);
    drawText(10, 580, "Lab 6: 3D Transformations");
    drawText(10, 560, (std::string("Mode: ") + currentMode).c_str());
    drawText(10, 540, (std::string("Projection: ") + (perspectiveProjection ? "Perspective" : "Axonometric")).c_str());
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    glutSwapBuffers();
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    if (perspectiveProjection) {
        gluPerspective(45.0, (double)width / height, 1.0, 10.0);
    } else {
        glOrtho(-2.0, 2.0, -2.0, 2.0, -10.0, 10.0);
    }
    
    glMatrixMode(GL_MODELVIEW);
}

void applyTransformation(const Matrix4x4& transformation) {
    currentTransformation = transformation * currentTransformation;
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        // Выбор фигур
        case '1': currentPolyhedron = createHexahedron(); break;
        case '2': currentPolyhedron = createIcosahedron(); break;
        
        // Аффинные преобразования
        case 't': applyTransformation(createTranslationMatrix(0.5, 0, 0)); break;
        case 'T': applyTransformation(createTranslationMatrix(-0.5, 0, 0)); break;
        case 's': applyTransformation(createScaleMatrix(1.2, 1.2, 1.2)); break;
        case 'S': applyTransformation(createScaleMatrix(0.8, 0.8, 0.8)); break;
        case 'x': applyTransformation(createRotationXMatrix(15)); break;
        case 'X': applyTransformation(createRotationXMatrix(-15)); break;
        case 'y': applyTransformation(createRotationYMatrix(15)); break;
        case 'Y': applyTransformation(createRotationYMatrix(-15)); break;
        case 'z': applyTransformation(createRotationZMatrix(15)); break;
        case 'Z': applyTransformation(createRotationZMatrix(-15)); break;
        case 'a': {
            Point3D p1(1, 1, 1);
            Point3D p2(-1, -1, -1);
            applyTransformation(createArbitraryRotationMatrix(p1, p2, 15.0));
            break;
        }
        case 'A': {
            Point3D p1(1, 1, 1);
            Point3D p2(-1, -1, -1);
            applyTransformation(createArbitraryRotationMatrix(p1, p2, -15.0));
            break;
        }
        
        // Отражение
        case 'm': applyTransformation(createReflectionMatrix('X')); break;
        case 'n': applyTransformation(createReflectionMatrix('Y')); break;
        case 'b': applyTransformation(createReflectionMatrix('Z')); break;
        
        // Масштабирование относительно центра
        case 'c': {
            Point3D center = currentPolyhedron.getCenter();
            applyTransformation(createTranslationMatrix(-center.x, -center.y, -center.z));
            applyTransformation(createScaleMatrix(1.5, 1.5, 1.5));
            applyTransformation(createTranslationMatrix(center.x, center.y, center.z));
            break;
        }
        case 'C': {
            Point3D center = currentPolyhedron.getCenter();
            applyTransformation(createTranslationMatrix(-center.x, -center.y, -center.z));
            applyTransformation(createScaleMatrix(0.7, 0.7, 0.7));
            applyTransformation(createTranslationMatrix(center.x, center.y, center.z));
            break;
        }
        
        // Проекции
        case 'p': 
            perspectiveProjection = !perspectiveProjection; 
            reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
            break;
        
        // Сброс
        case 'r': 
            rotationX = rotationY = 0.0f; 
            currentTransformation.identity();
            break;
            
        case 27: exit(0); break; // ESC
    }
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT: rotationY -= 5.0f; break;
        case GLUT_KEY_RIGHT: rotationY += 5.0f; break;
        case GLUT_KEY_UP: rotationX -= 5.0f; break;
        case GLUT_KEY_DOWN: rotationX += 5.0f; break;
    }
    glutPostRedisplay();
}

void printInstructions() {
    std::cout << "=== Управление ===" << std::endl;
    std::cout << "Фигуры: 1-Гексаэдр (куб), 2-Икосаэдр" << std::endl;
    std::cout << "Преобразования:" << std::endl;
    std::cout << "  t/T - смещение по X" << std::endl;
    std::cout << "  s/S - масштаб" << std::endl;
    std::cout << "  x/X, y/Y, z/Z - повороты" << std::endl;
    std::cout << "  a/A - поворот вокруг произвольной оси" << std::endl; // *** НОВАЯ ИНСТРУКЦИЯ ***
    std::cout << "  m,n,b - отражения (X,Y,Z плоскости)" << std::endl;
    std::cout << "  c/C - масштаб от центра" << std::endl;
    std::cout << "  p - переключение проекций" << std::endl;
    std::cout << "  r - сброс" << std::endl;
    std::cout << "Стрелки - вращение камеры" << std::endl;
    std::cout << "ESC - выход" << std::endl;
    std::cout << "==================" << std::endl;
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Lab 6 - 3D Transformations");
    
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    currentPolyhedron = createHexahedron();
    currentTransformation.identity();
    
    printInstructions();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    
    glutMainLoop();
    return 0;
}