#include <GL/glut.h>
#include <GL/gl.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Point3D {
public:
    double x, y, z, w;
    
    Point3D(double x = 0, double y = 0, double z = 0, double w = 1) 
        : x(x), y(y), z(z), w(w) {}
    
    Point3D operator+(const Point3D& other) const {
        return Point3D(x + other.x, y + other.y, z + other.z);
    }
    
    Point3D operator-(const Point3D& other) const {
        return Point3D(x - other.x, y - other.y, z - other.z);
    }
    
    Point3D operator*(double scalar) const {
        return Point3D(x * scalar, y * scalar, z * scalar);
    }
    
    bool operator==(const Point3D& other) const {
        const double epsilon = 1e-9;
        return std::abs(x - other.x) < epsilon &&
               std::abs(y - other.y) < epsilon &&
               std::abs(z - other.z) < epsilon;
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
std::string currentFilename = "";

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
    if (len < 1e-10) return Matrix4x4();
    
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

// Загрузка и сохранение OBJ файлов
bool loadOBJ(const std::string& filename, Polyhedron& poly) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Ошибка: не удалось открыть файл " << filename << std::endl;
        return false;
    }
    
    std::vector<Point3D> vertices;
    std::vector<std::vector<int>> faces;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "v") {
            double x, y, z;
            if (iss >> x >> y >> z) {
                vertices.push_back(Point3D(x, y, z));
            }
        } else if (type == "f") {
            std::vector<int> face;
            std::string vertex;
            while (iss >> vertex) {
                size_t pos = vertex.find('/');
                if (pos != std::string::npos) {
                    vertex = vertex.substr(0, pos);
                }
                try {
                    int idx = std::stoi(vertex);
                    if (idx > 0 && idx <= (int)vertices.size()) {
                        face.push_back(idx - 1); // OBJ использует 1-based индексацию
                    }
                } catch (...) {
                    // Игнорируем некорректные индексы
                }
            }
            if (!face.empty()) {
                faces.push_back(face);
            }
        }
    }
    
    file.close();
    
    if (vertices.empty()) {
        std::cout << "Ошибка: файл не содержит вершин" << std::endl;
        return false;
    }
    
    std::vector<Polygon> polygons;
    for (const auto& face : faces) {
        std::vector<Point3D> polygonPoints;
        for (int idx : face) {
            if (idx >= 0 && idx < (int)vertices.size()) {
                polygonPoints.push_back(vertices[idx]);
            }
        }
        if (!polygonPoints.empty()) {
            polygons.push_back(Polygon(polygonPoints));
        }
    }
    
    poly = Polyhedron(polygons);
    std::cout << "Загружено: " << vertices.size() << " вершин, " << faces.size() << " граней" << std::endl;
    return true;
}

bool saveOBJ(const std::string& filename, const Polyhedron& poly) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cout << "Ошибка: не удалось создать файл " << filename << std::endl;
        return false;
    }
    
    file << "# OBJ file generated by Lab 7\n";
    
    // Собираем все уникальные вершины
    std::vector<Point3D> vertices;
    
    for (const auto& polygon : poly.polygons) {
        for (const auto& point : polygon.points) {
            // Проверяем, есть ли уже такая вершина
            bool found = false;
            for (const auto& v : vertices) {
                if (point == v) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                vertices.push_back(point);
            }
        }
    }
    
    // Записываем вершины
    for (const auto& v : vertices) {
        file << "v " << v.x << " " << v.y << " " << v.z << "\n";
    }
    
    // Записываем грани
    for (const auto& polygon : poly.polygons) {
        file << "f";
        for (const auto& point : polygon.points) {
            // Находим индекс вершины
            for (size_t i = 0; i < vertices.size(); i++) {
                if (point == vertices[i]) {
                    file << " " << (i + 1);
                    break;
                }
            }
        }
        file << "\n";
    }
    
    file.close();
    std::cout << "Сохранено: " << vertices.size() << " вершин, " << poly.polygons.size() << " граней в " << filename << std::endl;
    return true;
}

// Построение фигуры вращения
Polyhedron createRevolutionSurface(const std::vector<Point3D>& generatrix, char axis, int divisions) {
    if (generatrix.empty() || divisions < 3) {
        return Polyhedron();
    }
    
    double angleStep = 360.0 / divisions;
    std::vector<std::vector<Point3D>> rotatedPoints;
    
    // Создаем точки для каждого угла вращения
    for (int i = 0; i < divisions; i++) {
        double angle = i * angleStep * M_PI / 180.0;
        std::vector<Point3D> rotated;
        
        for (const auto& point : generatrix) {
            Point3D rotatedPoint;
            switch (axis) {
                case 'X':
                case 'x':
                    rotatedPoint = Point3D(
                        point.x,
                        point.y * cos(angle) - point.z * sin(angle),
                        point.y * sin(angle) + point.z * cos(angle)
                    );
                    break;
                case 'Y':
                case 'y':
                    rotatedPoint = Point3D(
                        point.x * cos(angle) + point.z * sin(angle),
                        point.y,
                        -point.x * sin(angle) + point.z * cos(angle)
                    );
                    break;
                case 'Z':
                case 'z':
                    rotatedPoint = Point3D(
                        point.x * cos(angle) - point.y * sin(angle),
                        point.x * sin(angle) + point.y * cos(angle),
                        point.z
                    );
                    break;
                default:
                    rotatedPoint = point;
                    break;
            }
            rotated.push_back(rotatedPoint);
        }
        rotatedPoints.push_back(rotated);
    }
    
    // Создаем грани
    std::vector<Polygon> polygons;
    
    for (int i = 0; i < divisions; i++) {
        int next = (i + 1) % divisions;
        
        for (size_t j = 0; j < generatrix.size() - 1; j++) {
            std::vector<Point3D> quad;
            quad.push_back(rotatedPoints[i][j]);
            quad.push_back(rotatedPoints[next][j]);
            quad.push_back(rotatedPoints[next][j + 1]);
            quad.push_back(rotatedPoints[i][j + 1]);
            polygons.push_back(Polygon(quad));
        }
    }
    
    return Polyhedron(polygons);
}

// Построение графика двух переменных
double function1(double x, double y) {
    return sin(sqrt(x*x + y*y)) / (sqrt(x*x + y*y) + 1);
}

double function2(double x, double y) {
    return sin(x) * cos(y);
}

double function3(double x, double y) {
    return x*x + y*y;
}

Polyhedron createFunctionSurface(double (*func)(double, double), 
                                  double x0, double x1, double y0, double y1, 
                                  int xDivisions, int yDivisions) {
    if (xDivisions < 2 || yDivisions < 2) {
        return Polyhedron();
    }
    
    double xStep = (x1 - x0) / (xDivisions - 1);
    double yStep = (y1 - y0) / (yDivisions - 1);
    
    std::vector<std::vector<Point3D>> grid;
    
    // Создаем сетку точек
    for (int i = 0; i < yDivisions; i++) {
        std::vector<Point3D> row;
        double y = y0 + i * yStep;
        for (int j = 0; j < xDivisions; j++) {
            double x = x0 + j * xStep;
            double z = func(x, y);
            row.push_back(Point3D(x, y, z));
        }
        grid.push_back(row);
    }
    
    // Создаем грани (квадрики)
    std::vector<Polygon> polygons;
    
    for (int i = 0; i < yDivisions - 1; i++) {
        for (int j = 0; j < xDivisions - 1; j++) {
            std::vector<Point3D> quad;
            quad.push_back(grid[i][j]);
            quad.push_back(grid[i][j + 1]);
            quad.push_back(grid[i + 1][j + 1]);
            quad.push_back(grid[i + 1][j]);
            polygons.push_back(Polygon(quad));
        }
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
        glRotatef(35.264, 1.0, 0.0, 0.0);
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
    
    Polyhedron transformedPoly = currentPolyhedron;
    transformedPoly.transform(currentTransformation);
    drawPolyhedron(transformedPoly);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glColor3f(1.0, 1.0, 1.0);
    drawText(10, 580, "Lab 7: 3D Models");
    drawText(10, 560, (std::string("Mode: ") + currentMode).c_str());
    drawText(10, 540, (std::string("Projection: ") + (perspectiveProjection ? "Perspective" : "Axonometric")).c_str());
    if (!currentFilename.empty()) {
        drawText(10, 520, (std::string("File: ") + currentFilename).c_str());
    }
    
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
        case '1': 
            currentPolyhedron = createHexahedron(); 
            currentTransformation.identity();
            currentFilename = "";
            break;
        case '2': 
            currentPolyhedron = createIcosahedron(); 
            currentTransformation.identity();
            currentFilename = "";
            break;
        
        // Загрузка и сохранение
        case 'l': {
            std::cout << "Введите имя файла для загрузки: ";
            std::string filename;
            std::cin >> filename;
            if (loadOBJ(filename, currentPolyhedron)) {
                currentFilename = filename;
                currentTransformation.identity();
                std::cout << "Модель загружена успешно" << std::endl;
            }
            break;
        }
        case 'w': {
            std::string filename = currentFilename.empty() ? "model.obj" : currentFilename;
            std::cout << "Введите имя файла для сохранения (Enter для " << filename << "): ";
            std::string input;
            std::cin.ignore();
            std::getline(std::cin, input);
            if (!input.empty()) {
                filename = input;
            }
            // Применяем преобразования перед сохранением
            Polyhedron transformedPoly = currentPolyhedron;
            transformedPoly.transform(currentTransformation);
            if (saveOBJ(filename, transformedPoly)) {
                currentFilename = filename;
                std::cout << "Модель сохранена успешно (с примененными преобразованиями)" << std::endl;
            }
            break;
        }
        
        // Режим построения фигуры вращения
        case 'r': {
            currentMode = "REVOLUTION";
            std::cout << "Режим построения фигуры вращения" << std::endl;
            std::cout << "Использовать пример образующей? (y/n): ";
            char useExample;
            std::cin >> useExample;
            std::cin.ignore();
            
            std::vector<Point3D> generatrix;
            if (useExample == 'y' || useExample == 'Y') {
                generatrix = {
                    Point3D(0.5, -1.0, 0),
                    Point3D(0.7, -0.5, 0),
                    Point3D(0.8, 0.0, 0),
                    Point3D(0.7, 0.5, 0),
                    Point3D(0.5, 1.0, 0)
                };
                std::cout << "Используется пример образующей" << std::endl;
            } else {
                std::cout << "Введите количество точек образующей: ";
                int numPoints;
                std::cin >> numPoints;
                std::cout << "Введите координаты точек (x y z для каждой):" << std::endl;
                for (int i = 0; i < numPoints; i++) {
                    double x, y, z;
                    std::cin >> x >> y >> z;
                    generatrix.push_back(Point3D(x, y, z));
                }
            }
            
            std::cout << "Выберите ось вращения (x/y/z): ";
            char axis;
            std::cin >> axis;
            
            std::cout << "Введите количество разбиений (по умолчанию 20): ";
            std::string divInput;
            std::cin.ignore();
            std::getline(std::cin, divInput);
            int divisions = 20;
            if (!divInput.empty()) {
                divisions = std::stoi(divInput);
            }
            
            currentPolyhedron = createRevolutionSurface(generatrix, axis, divisions);
            currentTransformation.identity();
            currentFilename = "";
            currentMode = "VIEW";
            std::cout << "Фигура вращения создана" << std::endl;
            break;
        }
        case 'R': {
            rotationX = rotationY = 0.0f;
            currentTransformation.identity();
            break;
        }
        
        // Режим построения графика двух переменных
        case 's': {
            currentMode = "SURFACE";
            std::cout << "Режим построения графика двух переменных" << std::endl;
            std::cout << "Выберите функцию (1/2/3): ";
            int funcNum;
            std::cin >> funcNum;
            
            double (*func)(double, double) = nullptr;
            switch (funcNum) {
                case 1: func = function1; std::cout << "Выбрана функция: sin(sqrt(x^2+y^2))/(sqrt(x^2+y^2)+1)" << std::endl; break;
                case 2: func = function2; std::cout << "Выбрана функция: sin(x)*cos(y)" << std::endl; break;
                case 3: func = function3; std::cout << "Выбрана функция: x^2 + y^2" << std::endl; break;
                default: func = function1; break;
            }
            
            std::cout << "Введите диапазон X (x0 x1): ";
            double x0, x1;
            std::cin >> x0 >> x1;
            
            std::cout << "Введите диапазон Y (y0 y1): ";
            double y0, y1;
            std::cin >> y0 >> y1;
            
            std::cout << "Введите количество разбиений по X (по умолчанию 20): ";
            std::string xDivInput;
            std::cin.ignore();
            std::getline(std::cin, xDivInput);
            int xDivisions = 20;
            if (!xDivInput.empty()) {
                xDivisions = std::stoi(xDivInput);
            }
            
            std::cout << "Введите количество разбиений по Y (по умолчанию 20): ";
            std::string yDivInput;
            std::getline(std::cin, yDivInput);
            int yDivisions = 20;
            if (!yDivInput.empty()) {
                yDivisions = std::stoi(yDivInput);
            }
            
            currentPolyhedron = createFunctionSurface(func, x0, x1, y0, y1, xDivisions, yDivisions);
            currentTransformation.identity();
            currentFilename = "";
            currentMode = "VIEW";
            std::cout << "График функции создан" << std::endl;
            break;
        }
        case 'S': {
            applyTransformation(createScaleMatrix(0.8, 0.8, 0.8));
            break;
        }
        
        // Аффинные преобразования
        case 't': applyTransformation(createTranslationMatrix(0.5, 0, 0)); break;
        case 'T': applyTransformation(createTranslationMatrix(-0.5, 0, 0)); break;
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
    std::cout << "=== Управление Lab 7 ===" << std::endl;
    std::cout << "Фигуры:" << std::endl;
    std::cout << "  1 - Гексаэдр (куб)" << std::endl;
    std::cout << "  2 - Икосаэдр" << std::endl;
    std::cout << "Загрузка/Сохранение:" << std::endl;
    std::cout << "  l - загрузить OBJ файл" << std::endl;
    std::cout << "  w - сохранить модель в OBJ файл" << std::endl;
    std::cout << "Построение:" << std::endl;
    std::cout << "  r - построить фигуру вращения" << std::endl;
    std::cout << "  s - построить график двух переменных" << std::endl;
    std::cout << "Преобразования:" << std::endl;
    std::cout << "  t/T - смещение по X (вправо/влево)" << std::endl;
    std::cout << "  S - масштаб (уменьшение)" << std::endl;
    std::cout << "  x/X, y/Y, z/Z - повороты (положительный/отрицательный)" << std::endl;
    std::cout << "  a/A - поворот вокруг произвольной оси" << std::endl;
    std::cout << "  m,n,b - отражения (X,Y,Z плоскости)" << std::endl;
    std::cout << "  c/C - масштаб от центра (увеличение/уменьшение)" << std::endl;
    std::cout << "  p - переключение проекций (перспективная/аксонометрическая)" << std::endl;
    std::cout << "  R - сброс преобразований" << std::endl;
    std::cout << "Стрелки - вращение камеры" << std::endl;
    std::cout << "ESC - выход" << std::endl;
    std::cout << "========================" << std::endl;
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Lab 7 - 3D Models");
    
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

