#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// --- Глобальные переменные ---

// ID шейдерной программы
GLuint Program;

// ID атрибутов шейдера
GLint Attrib_pos;
GLint Attrib_tex;
GLint Attrib_norm;

// Uniforms для освещения и матриц
GLint Unif_model;
GLint Unif_view;
GLint Unif_proj;
GLint Unif_viewPos;
GLint Unif_lightDir;
GLint Unif_lightAmbient;
GLint Unif_lightDiffuse;
GLint Unif_lightSpecular;
GLint Unif_materialShininess;

// Камера
glm::vec3 cameraPos   = glm::vec3(0.0f, 2.0f, 10.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw   = -90.0f;
float pitch = 0.0f;
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;
float cameraSpeed = 5.0f;

// --- Структуры данных ---

struct Vec3 { GLfloat x, y, z; };
struct Vec2 { GLfloat u, v; };

// Структура для хранения сырых данных модели (CPU)
struct ModelData {
    std::vector<Vec3> vertices;
    std::vector<Vec2> texcoords;
    std::vector<Vec3> normals;
    int vertexCount;
};

// Структура для объекта на сцене (GPU + Transform)
struct GameObject {
    GLuint VAO;
    GLuint VBO_Pos;
    GLuint VBO_Tex;
    GLuint VBO_Norm;
    GLuint TextureID;
    int vertexCount;
    
    // Трансформации
    glm::vec3 position;
    glm::vec3 rotation; // Эйлеровы углы
    glm::vec3 scale;
    
    std::string name; // Для удобства
};

std::vector<GameObject> sceneObjects;

// --- Шейдеры (Phong + Directional Light) ---

const char* VertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec2 TexCoord;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // Позиция фрагмента в мире
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Нормаль пересчитываем с учетом масштаба (Normal Matrix)
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    TexCoord = aTexCoord;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* FragShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec2 TexCoord;
in vec3 Normal;

// Текстура объекта
uniform sampler2D texture1;

// Параметры камеры
uniform vec3 viewPos;

// Параметры Направленного источника света (Directional Light)
uniform vec3 lightDir;     // Направление света
uniform vec3 lightAmbient; // Фоновый свет
uniform vec3 lightDiffuse; // Рассеянный свет
uniform vec3 lightSpecular;// Зеркальный блик

// Параметры материала
uniform float materialShininess;

void main() {
    // 0. Базовые вектора
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    // lightDir указывает НА источник света (или инвертируем, если передаем направление лучей)
    // Здесь предполагаем, что lightDir - это вектор падения света, поэтому берем обратный для расчетов Phong
    vec3 lightDirection = normalize(-lightDir);

    // 1. Ambient (Фон)
    vec3 ambient = lightAmbient * texture(texture1, TexCoord).rgb;
  
    // 2. Diffuse (Рассеивание)
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = lightDiffuse * diff * texture(texture1, TexCoord).rgb;
    
    // 3. Specular (Блик Фонга)
    vec3 reflectDir = reflect(-lightDirection, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    vec3 specular = lightSpecular * spec; // Блик обычно белый
    
    // Итоговый цвет
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
)";

// --- Функции ---

void checkOpenGLerror() {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error! code " << err << std::endl;
    }
}

// Загрузка OBJ (Vertices, TexCoords, Normals)
bool LoadOBJ(const char* filename, ModelData& model) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return false;
    }

    std::vector<Vec3> temp_vertices;
    std::vector<Vec2> temp_texcoords;
    std::vector<Vec3> temp_normals;

    // Индексы для сборки
    std::vector<int> v_indices;
    std::vector<int> vt_indices;
    std::vector<int> vn_indices;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            Vec3 v; iss >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        } else if (prefix == "vt") {
            Vec2 vt; iss >> vt.u >> vt.v;
            temp_texcoords.push_back(vt);
        } else if (prefix == "vn") {
            Vec3 vn; iss >> vn.x >> vn.y >> vn.z;
            temp_normals.push_back(vn);
        } else if (prefix == "f") {
            std::string vertex_str;
            // OBJ может иметь 3 или 4 вершины на грань (триангуляцию здесь делаем простую)
            // Читаем все вершины грани
            std::vector<std::string> face_verts;
            while(iss >> vertex_str) face_verts.push_back(vertex_str);

            // Триангуляция (fan method): 0-1-2, 0-2-3...
            for(size_t i = 1; i < face_verts.size() - 1; i++) {
                // Функция разбора строки "v/vt/vn"
                auto parse_triple = [&](const std::string& str) {
                    std::stringstream ss(str);
                    std::string segment;
                    std::vector<std::string> segs;
                    while(std::getline(ss, segment, '/')) {
                        segs.push_back(segment);
                    }
                    
                    int v_idx = std::stoi(segs[0]) - 1;
                    v_indices.push_back(v_idx);

                    if (segs.size() > 1 && !segs[1].empty()) {
                        vt_indices.push_back(std::stoi(segs[1]) - 1);
                    } else {
                        vt_indices.push_back(-1); // Нет текстуры
                    }

                    if (segs.size() > 2 && !segs[2].empty()) {
                        vn_indices.push_back(std::stoi(segs[2]) - 1);
                    } else {
                        vn_indices.push_back(-1); // Нет нормали
                    }
                };

                parse_triple(face_verts[0]);
                parse_triple(face_verts[i]);
                parse_triple(face_verts[i+1]);
            }
        }
    }
    file.close();

    // Сборка финальных буферов (разворачивание индексов)
    model.vertices.clear();
    model.texcoords.clear();
    model.normals.clear();

    for (size_t i = 0; i < v_indices.size(); i++) {
        // Вершины
        model.vertices.push_back(temp_vertices[v_indices[i]]);
        
        // Текстуры
        if (vt_indices[i] >= 0 && vt_indices[i] < temp_texcoords.size())
            model.texcoords.push_back(temp_texcoords[vt_indices[i]]);
        else
            model.texcoords.push_back({0.0f, 0.0f});

        // Нормали
        if (vn_indices[i] >= 0 && vn_indices[i] < temp_normals.size())
            model.normals.push_back(temp_normals[vn_indices[i]]);
        else
            model.normals.push_back({0.0f, 1.0f, 0.0f}); // Заглушка
    }

    model.vertexCount = model.vertices.size();
    std::cout << "Loaded OBJ: " << filename << " Verts: " << model.vertexCount << std::endl;
    return true;
}

GLuint LoadTexture(const char* filename) {
    sf::Image image;
    if (!image.loadFromFile(filename)) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return 0;
    }
    image.flipVertically(); // OpenGL ожидает текстуру перевернутой по Y

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getSize().x, image.getSize().y, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);

    return texture;
}

void InitShader() {
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &VertexShaderSource, NULL);
    glCompileShader(vShader);
    
    // Проверка ошибок компиляции VS (упрощено)
    GLint success; char infoLog[512];
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
    if(!success) { glGetShaderInfoLog(vShader, 512, NULL, infoLog); std::cout << "VS Error: " << infoLog << std::endl; }

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &FragShaderSource, NULL);
    glCompileShader(fShader);
    
    // Проверка ошибок компиляции FS
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
    if(!success) { glGetShaderInfoLog(fShader, 512, NULL, infoLog); std::cout << "FS Error: " << infoLog << std::endl; }

    Program = glCreateProgram();
    glAttachShader(Program, vShader);
    glAttachShader(Program, fShader);
    glLinkProgram(Program);

    // Получаем локации атрибутов и юниформов
    Attrib_pos  = glGetAttribLocation(Program, "aPos");
    Attrib_tex  = glGetAttribLocation(Program, "aTexCoord");
    Attrib_norm = glGetAttribLocation(Program, "aNormal");

    Unif_model = glGetUniformLocation(Program, "model");
    Unif_view  = glGetUniformLocation(Program, "view");
    Unif_proj  = glGetUniformLocation(Program, "projection");
    
    Unif_viewPos       = glGetUniformLocation(Program, "viewPos");
    Unif_lightDir      = glGetUniformLocation(Program, "lightDir");
    Unif_lightAmbient  = glGetUniformLocation(Program, "lightAmbient");
    Unif_lightDiffuse  = glGetUniformLocation(Program, "lightDiffuse");
    Unif_lightSpecular = glGetUniformLocation(Program, "lightSpecular");
    Unif_materialShininess = glGetUniformLocation(Program, "materialShininess");

    glDeleteShader(vShader);
    glDeleteShader(fShader);
}

// Добавление объекта в сцену
void AddObject(const char* objPath, const char* texPath, glm::vec3 pos, glm::vec3 scale, glm::vec3 rot, std::string name) {
    ModelData data;
    if (LoadOBJ(objPath, data)) {
        GameObject obj;
        obj.vertexCount = data.vertexCount;
        obj.position = pos;
        obj.scale = scale;
        obj.rotation = rot;
        obj.name = name;

        glGenVertexArrays(1, &obj.VAO);
        glBindVertexArray(obj.VAO);

        // VBO Позиции
        glGenBuffers(1, &obj.VBO_Pos);
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_Pos);
        glBufferData(GL_ARRAY_BUFFER, data.vertices.size() * sizeof(Vec3), data.vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(Attrib_pos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(Attrib_pos);

        // VBO Текстурные координаты
        glGenBuffers(1, &obj.VBO_Tex);
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_Tex);
        glBufferData(GL_ARRAY_BUFFER, data.texcoords.size() * sizeof(Vec2), data.texcoords.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(Attrib_tex, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(Attrib_tex);

        // VBO Нормали
        glGenBuffers(1, &obj.VBO_Norm);
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_Norm);
        glBufferData(GL_ARRAY_BUFFER, data.normals.size() * sizeof(Vec3), data.normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(Attrib_norm, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(Attrib_norm);

        obj.TextureID = LoadTexture(texPath);

        glBindVertexArray(0);
        sceneObjects.push_back(obj);
    }
}

void InitScene() {
    // 1. Домик (Центр)
    AddObject("assets/house.obj", "assets/house.jpg", 
              glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.1f), glm::vec3(0.0f, 0.0f, 0.0f), "House");

    // 2. Сани (Справа)
    AddObject("assets/sleigh.obj", "assets/sleigh.jpg", 
              glm::vec3(-4.0f, -1.0f, 1.0f), glm::vec3(0.003f), glm::vec3(0.0f, -45.0f, 0.0f), "Sleigh");

    // 3. Воздушный шар (Слева вверху)
    AddObject("assets/balloon.obj", "assets/balloon.jpg", 
              glm::vec3(-3.0f, 2.0f, -2.0f), glm::vec3(0.1f), glm::vec3(0.0f, 0.0f, 0.0f), "Balloon");

    // 4. Леденец (Спереди слева)
    AddObject("assets/lollipop.obj", "assets/lollipop.jpg", 
              glm::vec3(-2.0f, -1.0f, 2.0f), glm::vec3(0.1f), glm::vec3(0.0f, 30.0f, 0.0f), "Lollipop");

    // 5. Дирижабль (Высоко справа)
    AddObject("assets/zeppelin.obj", "assets/zeppelin.jpg", 
              glm::vec3(4.0f, 2.0f, -3.0f), glm::vec3(0.01f), glm::vec3(0.0f, 90.0f, 0.0f), "Zeppelin");
}

void DrawScene(float aspect) {
    glUseProgram(Program);

    // --- 1. Установка освещения (Направленный свет) ---
    // Свет падает сверху и немного сбоку
    glm::vec3 lightDir = glm::vec3(-0.5f, -1.0f, -0.3f); 
    
    glUniform3fv(Unif_lightDir, 1, glm::value_ptr(lightDir));
    glUniform3f(Unif_lightAmbient,  0.2f, 0.2f, 0.2f); // Тусклый свет
    glUniform3f(Unif_lightDiffuse,  0.8f, 0.8f, 0.8f); // Основной свет
    glUniform3f(Unif_lightSpecular, 1.0f, 1.0f, 1.0f); // Блики
    glUniform3fv(Unif_viewPos, 1, glm::value_ptr(cameraPos));
    glUniform1f(Unif_materialShininess, 32.0f); // Коэффициент блеска

    // --- 2. Матрицы камеры ---
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    glUniformMatrix4fv(Unif_view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(Unif_proj, 1, GL_FALSE, glm::value_ptr(projection));

    // --- 3. Отрисовка всех объектов ---
    for (const auto& obj : sceneObjects) {
        // Формируем Model Matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, obj.position);
        model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1,0,0));
        model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0,1,0));
        model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0,0,1));
        model = glm::scale(model, obj.scale);

        glUniformMatrix4fv(Unif_model, 1, GL_FALSE, glm::value_ptr(model));

        // Текстура
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, obj.TextureID);

        // Рисуем
        glBindVertexArray(obj.VAO);
        glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount);
        glBindVertexArray(0);
    }

    glUseProgram(0);
}

void Cleanup() {
    for(auto& obj : sceneObjects) {
        glDeleteBuffers(1, &obj.VBO_Pos);
        glDeleteBuffers(1, &obj.VBO_Tex);
        glDeleteBuffers(1, &obj.VBO_Norm);
        glDeleteVertexArrays(1, &obj.VAO);
        glDeleteTextures(1, &obj.TextureID);
    }
    glDeleteProgram(Program);
}

int main() {
    sf::Window window(sf::VideoMode(800, 600), "Phong Lighting Lab", 
                     sf::Style::Default, sf::ContextSettings(24, 8, 0, 3, 3));
    window.setVerticalSyncEnabled(true);
    window.setMouseCursorVisible(false);
    window.setActive(true);
    
    glewInit();
    glEnable(GL_DEPTH_TEST);
    
    InitShader();
    InitScene(); // Загружает 5 объектов
    
    std::cout << "Scene Loaded. Objects: " << sceneObjects.size() << std::endl;
    std::cout << "Controls: WASD + Mouse, Shift/Space" << std::endl;

    sf::Clock clock;

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed || 
               (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)) {
                window.close();
            }
            if (event.type == sf::Event::Resized) { 
                glViewport(0, 0, event.size.width, event.size.height); 
            }
            
            // Обработка мыши
            if (event.type == sf::Event::MouseMoved) {
                float xpos = static_cast<float>(event.mouseMove.x);
                float ypos = static_cast<float>(event.mouseMove.y);

                if (firstMouse) {
                    lastX = xpos; lastY = ypos; firstMouse = false;
                }

                float xoffset = xpos - lastX;
                float yoffset = lastY - ypos;
                lastX = xpos; lastY = ypos;

                float sensitivity = 0.1f;
                xoffset *= sensitivity;
                yoffset *= sensitivity;

                yaw += xoffset;
                pitch += yoffset;

                if (pitch > 89.0f) pitch = 89.0f;
                if (pitch < -89.0f) pitch = -89.0f;

                glm::vec3 front;
                front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                front.y = sin(glm::radians(pitch));
                front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                cameraFront = glm::normalize(front);
            }
        }
        
        // Управление камерой
        float camSpeed = cameraSpeed * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) cameraPos += camSpeed * cameraFront;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) cameraPos -= camSpeed * cameraFront;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * camSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * camSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) cameraPos += camSpeed * cameraUp;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) cameraPos -= camSpeed * cameraUp;

        // Рендер
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspect = (float)window.getSize().x / (float)window.getSize().y;
        DrawScene(aspect);

        window.display();
    }
    
    Cleanup();
    return 0;
}