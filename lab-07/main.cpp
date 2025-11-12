#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <fstream>
#include <cmath>
#include <functional>

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
            w = 1.0;
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

Matrix4x4 createOrthographicMatrix(double left, double right, double bottom, double top, double zNear, double zFar) {
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
        else if (type == "f") {
            std::vector<Point3D> face;
            std::string token;
            while (iss >> token) {
                // Индексы могут быть вида "1", "1/2/3", "1//3"
                std::stringstream ss(token);
                std::string indexStr;
                std::getline(ss, indexStr, '/');
                int idx = std::stoi(indexStr);
                if (idx < 0) idx = vertices.size() + idx + 1;
                face.push_back(vertices[idx - 1]);
            }
            polygons.push_back(Polygon(face));
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
    for (const auto& polygon : polyhedron.polygons) {
        for (const auto& point : polygon.points) {
            vertices.push_back(point);
        }
    }

    for (const auto& v : vertices) {
        file << "v " << v.x << " " << v.y << " " << v.z << "\n";
    }

    int index = 1;
    for (const auto& polygon : polyhedron.polygons) {
        file << "f";
        for (size_t i = 0; i < polygon.points.size(); ++i) {
            file << " " << index++;
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
        for (const auto& p : profile) {
            double x = p.x, y = p.y, z = p.z;
            Point3D p1, p2;

            if (axis == 'y' || axis == 'Y') {
                p1 = Point3D(x * cos(theta1), y, x * sin(theta1));
                p2 = Point3D(x * cos(theta2), y, x * sin(theta2));
            } else if (axis == 'x' || axis == 'X') {
                p1 = Point3D(p.z * sin(theta1), p.y * cos(theta1), p.z * cos(theta1));
                p2 = Point3D(p.z * sin(theta2), p.y * cos(theta2), p.z * cos(theta2));
            } else { // ось Z
                p1 = Point3D(x * cos(theta1) - y * sin(theta1),
                             x * sin(theta1) + y * cos(theta1),
                             z);
                p2 = Point3D(x * cos(theta2) - y * sin(theta2),
                             x * sin(theta2) + y * cos(theta2),
                             z);
            }

            ring1.push_back(p1);
            ring2.push_back(p2);
        }

        for (size_t j = 0; j < profile.size() - 1; ++j) {
            polygons.push_back(Polygon({
                ring1[j], ring1[j + 1], ring2[j + 1], ring2[j]
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
            polygons.push_back(Polygon({
                points[idx],
                points[idx + 1],
                points[idx + steps + 2],
                points[idx + steps + 1]
            }));
        }
    }

    return Polyhedron(polygons);
}


void printInstructions() {
    std::cout << "=== Управление ===" << std::endl;
    std::cout << "Фигуры: 1-Гексаэдр (куб), 2-Икосаэдр" << std::endl;
    std::cout << "Преобразования:" << std::endl;
    std::cout << "  t/T - смещение по X" << std::endl;
    std::cout << "  s/S - масштаб" << std::endl;
    std::cout << "  x/X, y/Y, z/Z - повороты" << std::endl;
    std::cout << "  a/A - поворот вокруг произвольной оси" << std::endl;
    std::cout << "  m,n,b - отражения (X,Y,Z плоскости)" << std::endl;
    std::cout << "  c/C - масштаб от центра" << std::endl;
    std::cout << "  p - переключение проекций" << std::endl;
    std::cout << "  r - сброс" << std::endl;
    std::cout << "  L - загрузить модель из OBJ" << std::endl;
    std::cout << "  O - сохранить текущую модель в OBJ" << std::endl;
    std::cout << "  R - построить модель вращения гриба" << std::endl;
    std::cout << "  F - отобразить функцию" << std::endl; 
    std::cout << "Стрелки - вращение камеры" << std::endl;
    std::cout << "ESC - выход" << std::endl;
    std::cout << "==================" << std::endl;
}

int main() {
    const int WIDTH = 800;
    const int HEIGHT = 600;

    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Трехмерные преобразования");
    window.setFramerateLimit(60);
    
    Polyhedron currentPolyhedron = createHexahedron();
    float rotationX = 20.0f;
    float rotationY = -30.0f;
    bool perspectiveProjection = true;
    Matrix4x4 currentObjectTransformation;
    currentObjectTransformation.identity();
    std::string currentMode = "VIEW";

    printInstructions();

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed) {
                Matrix4x4 transformation;
                transformation.identity();

                switch (event.key.code) {
                    case sf::Keyboard::Escape: window.close(); break;
                    
                    case sf::Keyboard::Num1: currentPolyhedron = createHexahedron(); break;
                    case sf::Keyboard::Num2: currentPolyhedron = createIcosahedron(); break;
                    
                    case sf::Keyboard::T:
                        transformation = event.key.shift ? createTranslationMatrix(-0.5, 0, 0) : createTranslationMatrix(0.5, 0, 0);
                        break;
                    case sf::Keyboard::S:
                         transformation = event.key.shift ? createScaleMatrix(0.8, 0.8, 0.8) : createScaleMatrix(1.2, 1.2, 1.2);
                        break;
                    case sf::Keyboard::X:
                        transformation = event.key.shift ? createRotationXMatrix(-15) : createRotationXMatrix(15);
                        break;
                    case sf::Keyboard::Y:
                         transformation = event.key.shift ? createRotationYMatrix(-15) : createRotationYMatrix(15);
                        break;
                    case sf::Keyboard::Z:
                        transformation = event.key.shift ? createRotationZMatrix(-15) : createRotationZMatrix(15);
                        break;
                    case sf::Keyboard::A: {
                        Point3D p1(1, 1, 1), p2(-1, -1, -1);
                        transformation = event.key.shift ? createArbitraryRotationMatrix(p1, p2, -15.0) : createArbitraryRotationMatrix(p1, p2, 15.0);
                        break;
                    }
                    case sf::Keyboard::M: transformation = createReflectionMatrix('X'); break;
                    case sf::Keyboard::N: transformation = createReflectionMatrix('Y'); break;
                    case sf::Keyboard::B: transformation = createReflectionMatrix('Z'); break;
                    
                    case sf::Keyboard::C: {
                        Point3D center = currentPolyhedron.getCenter();
                        Matrix4x4 toOrigin = createTranslationMatrix(-center.x, -center.y, -center.z);
                        Matrix4x4 scale = event.key.shift ? createScaleMatrix(0.7, 0.7, 0.7) : createScaleMatrix(1.5, 1.5, 1.5);
                        Matrix4x4 fromOrigin = createTranslationMatrix(center.x, center.y, center.z);
                        transformation = fromOrigin * scale * toOrigin;
                        break;
                    }
                    
                    case sf::Keyboard::P: perspectiveProjection = !perspectiveProjection; break;
                    
                    case sf::Keyboard::R: {
                        if (!event.key.shift) {
                            rotationX = 20.0f; rotationY = -30.0f; 
                            currentObjectTransformation.identity();
                        } else {
                            int n;
                            char axis;
                            std::cout << "Введите количество разбиений (например, 36): ";
                            std::cin >> n;
                            std::cout << "Введите ось вращения (x, y или z): ";
                            std::cin >> axis;

                            // ---- Профиль гриба ----
                            std::vector<Point3D> profile = {
                                {0.0, 0.0, 0.0},   // низ ножки
                                {0.1, 0.0, 0.0},
                                {0.15, 0.4, 0.0},  // начало шляпки
                                {0.4, 0.45, 0.0},
                                {0.5, 0.5, 0.0},
                                {0.3, 0.55, 0.0},
                                {0.0, 0.6, 0.0}    // верх шляпки
                            };

                            currentPolyhedron = generateSurfaceOfRevolution(profile, axis, n);
                            break;
                        }
                        break;
                    }

                    case sf::Keyboard::F: {
                        double x0, x1, y0, y1;
                        int steps;
                        int funcChoice;

                        std::cout << "Введите диапазон X (x0 x1): ";
                        std::cin >> x0 >> x1;
                        std::cout << "Введите диапазон Y (y0 y1): ";
                        std::cin >> y0 >> y1;
                        std::cout << "Введите количество разбиений (например, 50): ";
                        std::cin >> steps;

                        std::cout << "Выберите функцию:\n";
                        std::cout << "1 - z = sin(sqrt(x^2 + y^2))\n";
                        std::cout << "2 - z = cos(x) * sin(y)\n";
                        std::cout << "3 - z = x^2 + y^2\n";
                        std::cin >> funcChoice;

                        std::function<double(double,double)> func;

                        switch (funcChoice) {
                            case 1: func = [](double x, double y){ return sin(sqrt(x*x + y*y)); }; break;
                            case 2: func = [](double x, double y){ return cos(x) * sin(y); }; break;
                            case 3: func = [](double x, double y){ return x*x + y*y; }; break;
                            default: func = [](double x, double y){ return 0.0; };
                        }

                        currentPolyhedron = generateFunctionSurface(func, x0, x1, y0, y1, steps);
                        break;
                    }

                    case sf::Keyboard::Up: rotationX -= 5.0f; break;
                    case sf::Keyboard::Down: rotationX += 5.0f; break;
                    case sf::Keyboard::Left: rotationY -= 5.0f; break;
                    case sf::Keyboard::Right: rotationY += 5.0f; break;

                    case sf::Keyboard::L: {
                        std::string path;
                        std::cout << "Введите имя файла OBJ для загрузки: ";
                        std::cin >> path;
                        currentPolyhedron = loadOBJ(path);
                        break;
                    }
                    case sf::Keyboard::O: {
                        std::string path;
                        std::cout << "Введите имя файла OBJ для сохранения: ";
                        std::cin >> path;
                        saveOBJ(currentPolyhedron, path);
                        break;
                    }

                    default: break;
                }
                currentObjectTransformation = transformation * currentObjectTransformation;
            }
        }

        window.clear(sf::Color::Black);

        // --- Этап 1: Создание матриц вида и проекции ---
        Matrix4x4 viewMatrix, projMatrix;

        // Матрица вида (камера)
        Matrix4x4 rotX = createRotationXMatrix(rotationX);
        Matrix4x4 rotY = createRotationYMatrix(rotationY);
        Matrix4x4 viewTranslation = createTranslationMatrix(0, 0, -3); // Отодвигаем камеру назад
        viewMatrix = viewTranslation * rotX * rotY;
        
        // Матрица проекции
        if (perspectiveProjection) {
            projMatrix = createPerspectiveMatrix(45.0, (double)WIDTH / HEIGHT, 0.1, 100.0);
        } else {
            projMatrix = createOrthographicMatrix(-2.0, 2.0, -2.0, 2.0, -10.0, 10.0);
        }

        // --- Этап 2: Отрисовка осей координат ---
        {
            std::vector<Point3D> axes_points = {
                {0,0,0}, {2,0,0}, // X
                {0,0,0}, {0,2,0}, // Y
                {0,0,0}, {0,0,2}  // Z
            };
            std::vector<sf::Color> axes_colors = {sf::Color::Red, sf::Color::Green, sf::Color::Blue};

            Matrix4x4 mvp = projMatrix * viewMatrix; // Для осей не применяем трансформацию объекта
            
            for(size_t i = 0; i < 3; ++i) {
                sf::Vector2f p1 = project(axes_points[i*2], mvp, WIDTH, HEIGHT);
                sf::Vector2f p2 = project(axes_points[i*2 + 1], mvp, WIDTH, HEIGHT);
                sf::Vertex line[] = { sf::Vertex(p1, axes_colors[i]), sf::Vertex(p2, axes_colors[i]) };
                window.draw(line, 2, sf::Lines);
            }
        }
        
        // --- Этап 3: Отрисовка многогранника ---
        Matrix4x4 mvp = projMatrix * viewMatrix * currentObjectTransformation;

        sf::Color colors[] = {
            sf::Color::Red, sf::Color::Green, sf::Color::Blue,
            sf::Color::Yellow, sf::Color::Magenta, sf::Color::Cyan,
            {128, 128, 255}, {255, 128, 0}, {128, 255, 128}
        };

        for (size_t i = 0; i < currentPolyhedron.polygons.size(); i++) {
            const Polygon& polygon = currentPolyhedron.polygons[i];
            if (polygon.points.empty()) continue;

            sf::VertexArray lines(sf::LineStrip);
            sf::VertexArray points(sf::Points);
            
            for (const Point3D& point : polygon.points) {
                sf::Vector2f screenPos = project(point, mvp, WIDTH, HEIGHT);
                lines.append(sf::Vertex(screenPos, colors[i % 9]));
                points.append(sf::Vertex(screenPos, colors[i % 9]));
            }
            // Замыкаем контур
            lines.append(lines[0]);
            
            window.draw(lines);
            window.draw(points);
        }

        window.display();
    }

    return 0;
}
