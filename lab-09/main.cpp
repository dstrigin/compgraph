#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include "lib/math_3d.h"
#include "lib/geometry.h"
#include "lib/renderer.h"
#include "lib/camera.h"
#include "lib/zbuffer.h"

void printInstructions() {
    std::cout << "=== Управление ===" << std::endl;
    std::cout << "Фигуры: 1-Гексаэдр (куб), 2-Икосаэдр, 3-Сцена с объектами" << std::endl;
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
    std::cout << "  q/Q - отдалить/приблизить камеру" << std::endl;
    std::cout << "  V - визуализация z-буфера" << std::endl;
    std::cout << "  W - переключение режима отрисовки (линии/z-буфер)" << std::endl;
    std::cout << "  4/5/6 - Режимы: Гуро(4), Фонг/Тун(5), Обычный(6)" << std::endl;
    std::cout << "Стрелки - вращение камеры" << std::endl;
    std::cout << "ESC - выход" << std::endl;
    std::cout << "==================" << std::endl;
}

struct SceneObject {
    Polyhedron poly;
    Matrix4x4 transform;
    sf::Color color;
};

std::vector<SceneObject> createTestScene() {
    std::vector<SceneObject> objects;
    
    SceneObject obj1;
    obj1.poly = createHexahedron();
    obj1.transform = createTranslationMatrix(-1.5, 0, 0) * createScaleMatrix(0.7, 0.7, 0.7);
    obj1.color = sf::Color::Red;
    objects.push_back(obj1);
    
    SceneObject obj2;
    obj2.poly = createHexahedron();
    obj2.transform = createTranslationMatrix(0, 0, -1.5) * createScaleMatrix(0.8, 0.8, 0.8);
    obj2.color = sf::Color::Green;
    objects.push_back(obj2);
    
    SceneObject obj3;
    obj3.poly = createHexahedron();
    obj3.transform = createTranslationMatrix(1.5, 0, 1.0) * createScaleMatrix(0.6, 0.6, 0.6);
    obj3.color = sf::Color::Blue;
    objects.push_back(obj3);
    
    SceneObject obj4;
    obj4.poly = createIcosahedron();
    obj4.transform = createTranslationMatrix(0, 1.5, 0.5) * createScaleMatrix(0.5, 0.5, 0.5);
    obj4.color = sf::Color::Yellow;
    objects.push_back(obj4);
    
    return objects;
}

int main() {
    const int WIDTH = 800;
    const int HEIGHT = 600;

    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Трехмерные преобразования");
    window.setFramerateLimit(60);
    
    Polyhedron currentPolyhedron = createHexahedron();
    Camera camera(Point3D(0, 1, 5), Point3D(0, 0, 0));
    ZBuffer zbuffer(WIDTH, HEIGHT);

    Light mainLight;
    mainLight.position = Point3D(5, 5, 5); // Источник света
    mainLight.color = sf::Color::White;
    mainLight.intensity = 1.0f;

    enum RenderMode { GOURAUD, PHONG_TOON, FLAT, SCENE };
    RenderMode renderMode = FLAT;
    
    bool perspectiveProjection = true;
    bool useZBuffer = false;
    bool backfaceCulling = true;
    bool showZBufferViz = false;
    int sceneMode = 0;
    
    Matrix4x4 currentObjectTransformation;
    currentObjectTransformation.identity();
    std::string currentMode = "VIEW";
    
    std::vector<SceneObject> scene;

    printInstructions();
    Point3D viewDirection(0, 0, 1);

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
                    
                    case sf::Keyboard::Num1: 
                        currentPolyhedron = createHexahedron(); 
                        sceneMode = 0;
                        break;
                    case sf::Keyboard::Num2: 
                        currentPolyhedron = createIcosahedron(); 
                        sceneMode = 0;
                        break;
                    case sf::Keyboard::Num3:
                        scene = createTestScene();
                        sceneMode = 1;
                        std::cout << "Сцена с несколькими объектами загружена" << std::endl;
                        break;

                    case sf::Keyboard::Num4: 
                        renderMode = GOURAUD; 
                        std::cout << "Режим: Гуро (Lambert)" << std::endl;
                        useZBuffer = true;
                        break;
                    case sf::Keyboard::Num5: 
                        renderMode = PHONG_TOON;
                        std::cout << "Режим: Фонг (Toon)" << std::endl;
                        useZBuffer = true;
                        break;
                    case sf::Keyboard::Num6: 
                        renderMode = FLAT;
                        std::cout << "Режим: Плоская заливка" << std::endl;
                        sceneMode = 0;
                        break;
                    
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
                    
                    case sf::Keyboard::C: {
                        Point3D center = currentPolyhedron.getCenter();
                        Matrix4x4 toOrigin = createTranslationMatrix(-center.x, -center.y, -center.z);
                        Matrix4x4 scale = event.key.shift ? createScaleMatrix(0.7, 0.7, 0.7) : createScaleMatrix(1.5, 1.5, 1.5);
                        Matrix4x4 fromOrigin = createTranslationMatrix(center.x, center.y, center.z);
                        transformation = fromOrigin * scale * toOrigin;
                        break;
                    }
                    
                    case sf::Keyboard::P: perspectiveProjection = !perspectiveProjection; break;
                    
                    case sf::Keyboard::W:
                        useZBuffer = !useZBuffer;
                        std::cout << "Режим отрисовки: " << (useZBuffer ? "Z-Buffer" : "Линии") << std::endl;
                        break;
                    
                    case sf::Keyboard::V:
                        showZBufferViz = !showZBufferViz;
                        std::cout << "Z-buffer visualization: " << (showZBufferViz ? "ON" : "OFF") << std::endl;
                        break;
                    
                    case sf::Keyboard::R: {
                        if (!event.key.shift) {
                            camera = Camera(Point3D(0, 1, 5), Point3D(0, 0, 0));
                            currentObjectTransformation.identity();
                        } else {
                            int n;
                            char axis;
                            std::cout << "Введите количество разбиений (например, 36): ";
                            std::cin >> n;
                            std::cout << "Введите ось вращения (x, y или z): ";
                            std::cin >> axis;

                            std::vector<Point3D> profile = {
                                {0.0, 0.0, 0.0},
                                {0.1, 0.0, 0.0},
                                {0.15, 0.4, 0.0},
                                {0.4, 0.45, 0.0},
                                {0.5, 0.5, 0.0},
                                {0.3, 0.55, 0.0},
                                {0.0, 0.6, 0.0}
                            };

                            currentPolyhedron = generateSurfaceOfRevolution(profile, axis, n);
                            sceneMode = 0;
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
                        sceneMode = 0;
                        break;
                    }

                    case sf::Keyboard::Up: camera.rotateAroundTarget(0, 5.0f); break;
                    case sf::Keyboard::Down: camera.rotateAroundTarget(0, -5.0f); break;
                    case sf::Keyboard::Left: camera.rotateAroundTarget(-5.0f, 0); break;
                    case sf::Keyboard::Right: camera.rotateAroundTarget(5.0f, 0); break;

                    case sf::Keyboard::Q: 
                        camera.radius += event.key.shift ? -0.5f : 0.5f;
                        if (camera.radius < 1.0f) camera.radius = 1.0f;
                        camera.updatePosition();
                        break;

                    case sf::Keyboard::L: {
                        std::string path;
                        std::cout << "Введите имя файла OBJ для загрузки: ";
                        std::cin >> path;
                        currentPolyhedron = loadOBJ(path);
                        sceneMode = 0;
                        break;
                    }
                    case sf::Keyboard::O: {
                        std::string path;
                        std::cout << "Введите имя файла OBJ для сохранения: ";
                        std::cin >> path;
                        saveOBJ(currentPolyhedron, path);
                        break;
                    }
                    
                    case sf::Keyboard::U: event.key.shift ? viewDirection.x += 0.1 : viewDirection.x -= 0.1; break;
                    case sf::Keyboard::I: event.key.shift ? viewDirection.y += 0.1 : viewDirection.y -= 0.1; break;
                    case sf::Keyboard::H: event.key.shift ? viewDirection.z += 0.1 : viewDirection.z -= 0.1; break;

                    default: break;
                }
                currentObjectTransformation = transformation * currentObjectTransformation;
            }
        }

        window.clear(sf::Color::Black);

        Matrix4x4 viewMatrix, projMatrix;

        viewMatrix = camera.getViewMatrix();
        
        if (perspectiveProjection) {
            projMatrix = createPerspectiveMatrix(45.0, (double)WIDTH / HEIGHT, 0.1, 100.0);
        } else {
            projMatrix = createAxonometricMatrix(-2.0, 2.0, -2.0, 2.0, -10.0, 10.0);
        }

        if (useZBuffer) {
            zbuffer.clear();
            
            std::map<Point3D, Point3D> vertexNormals;
            if (sceneMode == 0) {
                vertexNormals = calculateSmoothNormals(currentPolyhedron);
            }

            Matrix4x4 viewMatrix = camera.getViewMatrix();

            if (sceneMode == 1) {
                for (auto& obj : scene) {
                    Matrix4x4 modelMatrix = currentObjectTransformation * obj.transform;
                    Matrix4x4 mvp = projMatrix * viewMatrix * modelMatrix;
                    
                    std::map<Point3D, Point3D> objNormals;
                    if (renderMode == GOURAUD || renderMode == PHONG_TOON) {
                        objNormals = calculateSmoothNormals(obj.poly);
                    }

                    for (const auto& polygon : obj.poly.polygons) {
                        if (polygon.points.size() < 3) continue;

                        for (size_t i = 1; i < polygon.points.size() - 1; i++) {
                            Point3D p1 = polygon.points[0];
                            Point3D p2 = polygon.points[i];
                            Point3D p3 = polygon.points[i + 1];

                            if (renderMode == GOURAUD) {
                                zbuffer.rasterizeTriangleGouraud(
                                    p1, p2, p3,
                                    objNormals[p1], objNormals[p2], objNormals[p3],
                                    obj.color, mvp, modelMatrix, mainLight
                                );
                            } 
                            else if (renderMode == PHONG_TOON) {
                                zbuffer.rasterizeTrianglePhongToon(
                                    p1, p2, p3,
                                    objNormals[p1], objNormals[p2], objNormals[p3],
                                    obj.color, mvp, modelMatrix, mainLight
                                );
                            } 
                            else {
                                zbuffer.rasterizeTriangle(p1, p2, p3, obj.color, mvp, backfaceCulling);
                            }
                        }
                    }
                }
            } else {
                Matrix4x4 modelMatrix = currentObjectTransformation;
                Matrix4x4 mvp = projMatrix * viewMatrix * modelMatrix;
                
                sf::Color colors[] = { sf::Color::Red, sf::Color::Green, sf::Color::Blue, sf::Color::Yellow, sf::Color::Cyan, sf::Color::Magenta };
                int colorIdx = 0;

                for (const auto& polygon : currentPolyhedron.polygons) {
                    if (polygon.points.size() < 3) continue;

                    sf::Color faceColor = colors[colorIdx % 6];
                    colorIdx++;

                    for (size_t i = 1; i < polygon.points.size() - 1; i++) {
                        Point3D p1 = polygon.points[0];
                        Point3D p2 = polygon.points[i];
                        Point3D p3 = polygon.points[i + 1];

                        if (renderMode == GOURAUD) {
                            zbuffer.rasterizeTriangleGouraud(
                                p1, p2, p3,
                                vertexNormals[p1], vertexNormals[p2], vertexNormals[p3],
                                faceColor, mvp, modelMatrix, mainLight
                            );
                        } 
                        else if (renderMode == PHONG_TOON) {
                            zbuffer.rasterizeTrianglePhongToon(
                                p1, p2, p3,
                                vertexNormals[p1], vertexNormals[p2], vertexNormals[p3],
                                faceColor, mvp, modelMatrix, mainLight
                            );
                        } 
                        else {
                            zbuffer.rasterizeTriangle(p1, p2, p3, faceColor, mvp, backfaceCulling);
                        }
                    }
                }
            }
            
            const sf::Image& frameImage = showZBufferViz ? zbuffer.getZBufferVisualization() : zbuffer.getFrameBuffer();
            sf::Texture texture;
            texture.loadFromImage(frameImage);
            sf::Sprite sprite(texture);
            window.draw(sprite);
            
        } else {
            {
                Matrix4x4 mvp = projMatrix * viewMatrix; 

                std::vector<Point3D> axes_points = {
                    {0,0,0}, {2,0,0},
                    {0,0,0}, {0,2,0},
                    {0,0,0}, {0,0,2}
                };
                std::vector<sf::Color> axes_colors = {sf::Color::Red, sf::Color::Green, sf::Color::Blue};
                
                for(size_t i = 0; i < 3; ++i) {
                    sf::Vector2f p1 = project(axes_points[i*2], mvp, WIDTH, HEIGHT);
                    sf::Vector2f p2 = project(axes_points[i*2 + 1], mvp, WIDTH, HEIGHT);
                    sf::Vertex line[] = { sf::Vertex(p1, axes_colors[i]), sf::Vertex(p2, axes_colors[i]) };
                    window.draw(line, 2, sf::Lines);
                }
            }
            
            if (sceneMode == 1) {
                for (auto& obj : scene) {
                    Polyhedron transformed = obj.poly;
                    Matrix4x4 modelMatrix = currentObjectTransformation * obj.transform;
                    transformed.transform(modelMatrix);
                    transformed.transform(viewMatrix);

                    for (const Polygon& polygon : transformed.polygons) {
                        if (polygon.points.empty()) continue;

                        Point3D normal = polygon.getNormal();
                        Point3D center(0,0,0);
                        for(const auto& p : polygon.points) center = center + p;
                        center = center * (1.0 / polygon.points.size());
                        Point3D viewDir = (Point3D(0,0,0) - center).normalize();
                        double dotVal = normal.dot(viewDir);
                        if (dotVal <= 0) continue;

                        sf::VertexArray lines(sf::LineStrip);
                        for (const Point3D& point : polygon.points) {
                            sf::Vector2f screenPos = project(point, projMatrix, WIDTH, HEIGHT);
                            lines.append(sf::Vertex(screenPos, obj.color));
                        }
                        if (!polygon.points.empty()) {
                            sf::Vector2f screenPos = project(polygon.points[0], projMatrix, WIDTH, HEIGHT);
                            lines.append(sf::Vertex(screenPos, obj.color));
                        }
                        window.draw(lines);
                    }
                }
            } else {
                Polyhedron transformed = currentPolyhedron;
                transformed.transform(currentObjectTransformation);
                transformed.transform(viewMatrix);

                std::sort(transformed.polygons.begin(), transformed.polygons.end(), 
                    [](const Polygon& a, const Polygon& b) {
                        double zA = 0, zB = 0;
                        for(const auto& p : a.points) zA += p.z;
                        for(const auto& p : b.points) zB += p.z;
                        return (zA / a.points.size()) < (zB / b.points.size());
                    });

                sf::Color colors[] = {
                    sf::Color::Red, sf::Color::Green, sf::Color::Blue,
                    sf::Color::Yellow, sf::Color::Magenta, sf::Color::Cyan,
                    {128, 128, 255}, {255, 128, 0}, {128, 255, 128}
                };

                int colorIndex = 0;
                for (const Polygon& polygon : transformed.polygons) {
                    if (polygon.points.empty()) continue;

                    Point3D normal = polygon.getNormal();
                    
                    Point3D center(0,0,0);
                    for(const auto& p : polygon.points) center = center + p;
                    center = center * (1.0 / polygon.points.size());
                    
                    Point3D viewDir = (Point3D(0,0,0) - center).normalize();

                    double dotVal = normal.dot(viewDir);

                    if (dotVal <= 0) continue; 

                    sf::VertexArray lines(sf::LineStrip);
                    
                    for (const Point3D& point : polygon.points) {
                        sf::Vector2f screenPos = project(point, projMatrix, WIDTH, HEIGHT);
                        lines.append(sf::Vertex(screenPos, colors[colorIndex % 9]));
                    }
                    if (!polygon.points.empty()) {
                        sf::Vector2f screenPos = project(polygon.points[0], projMatrix, WIDTH, HEIGHT);
                        lines.append(sf::Vertex(screenPos, colors[colorIndex % 9]));
                    }
                    
                    window.draw(lines);
                    colorIndex++;
                }
            }
        }

        window.display();
    }
    

    return 0;
}
