#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ID шейдерной программы
GLuint Program;
// ID вершинных атрибутов
GLint Attrib_vertex;
GLint Attrib_texcoord;
GLint Attrib_instanceMatrix; // Атрибут для матрицы инстанса
// ID буферов для центральной модели
GLuint VBO_Center;
GLuint TexCoordVBO_Center;
GLuint Texture_Center;
// ID буферов для орбитальных моделей
GLuint VBO_Orbit;
GLuint TexCoordVBO_Orbit;
GLuint Texture_Orbit;
GLuint InstanceVBO_Orbit; // Буфер для данных инстансов

// камера
glm::vec3 cameraPos   = glm::vec3(0.0f, 5.0f, 20.0f); 
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw   = -90.0f;
float pitch = -10.0f;
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;
float cameraSpeed = 10.0f; 

// Структура для вершины
struct Vertex {
    GLfloat x, y, z;
};

// Структура для текстурных координат
struct TexCoord {
    GLfloat u, v;
};

// Структура для хранения загруженной модели
struct ModelData {
    std::vector<Vertex> vertices;
    std::vector<TexCoord> texcoords;
    int vertexCount;
};

// Структура для планеты
struct Planet {
    float orbitRadius;      
    float orbitSpeed;       
    float rotationSpeed;    
    float size;            
    float orbitAngle;      
    float rotationAngle;  
};

ModelData centerModel;
ModelData orbitModel;

std::vector<Planet> planets;
std::vector<glm::mat4> instanceMatrices; // Массив матриц для инстансов

bool centerLoaded = false;
bool orbitLoaded = false;

// Исходный код вершинного шейдера с поддержкой инстансинга
const char* VertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 coord;
layout (location = 1) in vec2 texcoord;
layout (location = 2) in mat4 instanceMatrix; // Матрица трансформации для каждого инстанса

out vec2 v_texcoord;

uniform mat4 u_view;
uniform mat4 u_projection;

void main() {
    gl_Position = u_projection * u_view * instanceMatrix * vec4(coord, 1.0);
    v_texcoord = texcoord;
}
)";

// Исходный код фрагментного шейдера
const char* FragShaderSource = R"(
#version 330 core
in vec2 v_texcoord;
out vec4 color;

uniform sampler2D u_texture;

void main() {
    color = texture(u_texture, v_texcoord);
}
)";

void checkOpenGLerror() {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error! code " << err << std::endl;
    }
}

// Функция загрузки OBJ файла
bool LoadOBJ(const char* filename, ModelData& model) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return false;
    }

    std::vector<Vertex> temp_vertices; 
    std::vector<TexCoord> temp_texcoords;
    
    std::vector<int> vertex_indices;
    std::vector<int> texcoord_indices;

    std::string line;
    int face_count = 0;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            Vertex v;
            iss >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        }
        else if (prefix == "vt") {
            TexCoord tc;
            iss >> tc.u >> tc.v;
            temp_texcoords.push_back(tc);
        }
        else if (prefix == "f") {
            std::vector<int> face_v_indices;
            std::vector<int> face_vt_indices;
            
            std::string vertex_data;
            while (iss >> vertex_data) {
                std::istringstream vertex_stream(vertex_data);
                std::string index_str;
                int v_idx = -1, vt_idx = -1;
                
                if (std::getline(vertex_stream, index_str, '/')) {
                    if (!index_str.empty()) {
                        v_idx = std::stoi(index_str) - 1;
                    }
                }
                if (std::getline(vertex_stream, index_str, '/')) {
                    if (!index_str.empty()) {
                        vt_idx = std::stoi(index_str) - 1;
                    }
                }
                
                face_v_indices.push_back(v_idx);
                face_vt_indices.push_back(vt_idx);
            }
            
            // Триангуляция грани
            for (size_t i = 1; i < face_v_indices.size() - 1; i++) {
                vertex_indices.push_back(face_v_indices[0]);
                texcoord_indices.push_back(face_vt_indices[0]);
                
                vertex_indices.push_back(face_v_indices[i]);
                texcoord_indices.push_back(face_vt_indices[i]);
                
                vertex_indices.push_back(face_v_indices[i + 1]);
                texcoord_indices.push_back(face_vt_indices[i + 1]);
            }
            face_count++;
        }
    }
    file.close();

    std::cout << "Parsed OBJ file:" << std::endl;
    std::cout << "  Vertices: " << temp_vertices.size() << std::endl;
    std::cout << "  Texcoords: " << temp_texcoords.size() << std::endl;
    std::cout << "  Faces: " << face_count << std::endl;

    // Сборка финальных массивов
    model.vertices.clear();
    model.texcoords.clear();
    
    for (size_t i = 0; i < vertex_indices.size(); i++) {
        int v_idx = vertex_indices[i];
        if (v_idx >= 0 && v_idx < temp_vertices.size()) {
            model.vertices.push_back(temp_vertices[v_idx]);
            
            int vt_idx = texcoord_indices[i];
            if (vt_idx >= 0 && vt_idx < temp_texcoords.size()) {
                model.texcoords.push_back(temp_texcoords[vt_idx]);
            } else {
                TexCoord tc;
                tc.u = (temp_vertices[v_idx].x + 1.0f) * 0.5f;
                tc.v = (temp_vertices[v_idx].y + 1.0f) * 0.5f;
                model.texcoords.push_back(tc);
            }
        }
    }
    
    model.vertexCount = model.vertices.size();
    
    std::cout << "Final vertex count: " << model.vertexCount << std::endl;
    
    if (model.vertexCount == 0) {
        std::cerr << "WARNING: No vertices loaded!" << std::endl;
        return false;
    }
    
    return true;
}

// Функция загрузки текстуры 
GLuint LoadTexture(const char* filename) {
    sf::Image image;
    if (!image.loadFromFile(filename)) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
    }
    
    image.flipVertically();
    
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getSize().x, image.getSize().y, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);
    
    std::cout << "Loaded texture: " << filename << std::endl;
    
    return texture;
}

void ShaderLog(unsigned int shader) {
    int infologLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
    if (infologLen > 1) {
        int charsWritten = 0;
        std::vector<char> infoLog(infologLen);
        glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog.data());
        std::cout << "InfoLog: " << infoLog.data() << std::endl;
    }
}

void InitShader() {
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &VertexShaderSource, NULL);
    glCompileShader(vShader);
    ShaderLog(vShader);
    checkOpenGLerror();

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &FragShaderSource, NULL);
    glCompileShader(fShader);
    ShaderLog(fShader);
    checkOpenGLerror();

    Program = glCreateProgram();
    glAttachShader(Program, vShader);
    glAttachShader(Program, fShader);
    glLinkProgram(Program);
    
    int link_ok;
    glGetProgramiv(Program, GL_LINK_STATUS, &link_ok);
    if (!link_ok) {
        std::cout << "error attach shaders \n";
        return;
    }
    
    Attrib_vertex = glGetAttribLocation(Program, "coord");
    if (Attrib_vertex == -1) {
        std::cout << "could not bind attrib coord" << std::endl;
        return;
    }
    
    Attrib_texcoord = glGetAttribLocation(Program, "texcoord");
    if (Attrib_texcoord == -1) {
        std::cout << "could not bind attrib texcoord" << std::endl;
        return;
    }
    
    checkOpenGLerror();
}

void InitSolarSystem() {
    planets.clear();
    instanceMatrices.clear();
    
    // Разные размеры объектов
    float customSizes[] = {0.05f, 0.09f, 0.10f, 0.15f, 0.20f};
    // Разные радиусы орбит (как в настоящей солнечной системе)
    float orbitRadii[] = {8.0f, 12.0f, 16.0f, 20.0f, 24.0f};
    // Разные скорости вращения по орбите (ближе - быстрее)
    float orbitSpeeds[] = {1.2f, 1.0f, 0.8f, 0.6f, 0.4f};
    // Разные скорости вращения вокруг своей оси
    float rotationSpeeds[] = {2.0f, 1.5f, 1.8f, 1.2f, 1.0f};
    
    for(int i = 0; i < 5; i++) {
        Planet p;
        p.orbitRadius = orbitRadii[i];        // Каждая планета на своей орбите
        p.orbitSpeed = orbitSpeeds[i];        // Разные скорости обращения
        p.rotationSpeed = rotationSpeeds[i];  // Разные скорости вращения
        p.size = customSizes[i];              // Разные размеры
        p.orbitAngle = (M_PI * 2 / 5) * i;    // Начальное положение на орбите
        p.rotationAngle = 0.0f;
        planets.push_back(p);
        
        // Создаем начальную матрицу для каждого инстанса
        instanceMatrices.push_back(glm::mat4(1.0f));
    }
    
    std::cout << "Solar system initialized with " << planets.size() << " orbiting objects" << std::endl;
    std::cout << "Orbit radii: 5.0, 8.0, 11.0, 14.0, 17.0 units" << std::endl;
}

void LoadCenterModel(const char* objFile, const char* textureFile) {
    std::cout << "Loading center model..." << std::endl;
    if (LoadOBJ(objFile, centerModel)) {
        glGenBuffers(1, &VBO_Center);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_Center);
        glBufferData(GL_ARRAY_BUFFER, centerModel.vertices.size() * sizeof(Vertex), 
                     centerModel.vertices.data(), GL_STATIC_DRAW);
        
        glGenBuffers(1, &TexCoordVBO_Center);
        glBindBuffer(GL_ARRAY_BUFFER, TexCoordVBO_Center);
        glBufferData(GL_ARRAY_BUFFER, centerModel.texcoords.size() * sizeof(TexCoord), 
                     centerModel.texcoords.data(), GL_STATIC_DRAW);
        
        Texture_Center = LoadTexture(textureFile);
        
        centerLoaded = true;
        std::cout << "Center model loaded successfully!" << std::endl;
        checkOpenGLerror();
    }
}

void LoadOrbitModel(const char* objFile, const char* textureFile) {
    std::cout << "Loading orbit model..." << std::endl;
    if (LoadOBJ(objFile, orbitModel)) {
        glGenBuffers(1, &VBO_Orbit);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_Orbit);
        glBufferData(GL_ARRAY_BUFFER, orbitModel.vertices.size() * sizeof(Vertex), 
                     orbitModel.vertices.data(), GL_STATIC_DRAW);
        
        glGenBuffers(1, &TexCoordVBO_Orbit);
        glBindBuffer(GL_ARRAY_BUFFER, TexCoordVBO_Orbit);
        glBufferData(GL_ARRAY_BUFFER, orbitModel.texcoords.size() * sizeof(TexCoord), 
                     orbitModel.texcoords.data(), GL_STATIC_DRAW);
        
        Texture_Orbit = LoadTexture(textureFile);
        
        // Создаем буфер для инстансов
        glGenBuffers(1, &InstanceVBO_Orbit);
        
        orbitLoaded = true;
        std::cout << "Orbit model loaded successfully!" << std::endl;
        checkOpenGLerror();
    }
}

void UpdatePlanets(float deltaTime) {
    for (size_t i = 0; i < planets.size(); i++) {
        // Обновляем угол на орбите
        planets[i].orbitAngle += planets[i].orbitSpeed * deltaTime;
        // Обновляем угол поворота вокруг своей оси
        planets[i].rotationAngle += planets[i].rotationSpeed * deltaTime;
        
        // Вычисляем позицию на орбите
        float x = planets[i].orbitRadius * cos(planets[i].orbitAngle);
        float z = planets[i].orbitRadius * sin(planets[i].orbitAngle);
        float y = -1.5; // Все на одной плоскости
        
        // Создаем матрицу трансформации для инстанса
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, z));
        model = glm::rotate(model, planets[i].rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(planets[i].size));
        
        instanceMatrices[i] = model;
    }
}

void DrawCenterModel(float aspect) {
    if (!centerLoaded) return;
    
    glUseProgram(Program);
    
    static float rotationAngle = 0.0f;
    rotationAngle += 0.05f * 0.016f;
    
    // Матрицы преобразования
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 300.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    
    glm::mat4 modelMat = glm::mat4(1.0f);
    modelMat = glm::rotate(modelMat, rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::scale(modelMat, glm::vec3(0.10f));
    
    // Для центральной модели используем обычный рендеринг без инстансинга
    GLint view_loc = glGetUniformLocation(Program, "u_view");
    GLint proj_loc = glGetUniformLocation(Program, "u_projection");
    
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(projection));
    
    // Привязываем текстуру
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture_Center);
    GLint texture_loc = glGetUniformLocation(Program, "u_texture");
    glUniform1i(texture_loc, 0);
    
    // Вершины
    glEnableVertexAttribArray(Attrib_vertex);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Center);
    glVertexAttribPointer(Attrib_vertex, 3, GL_FLOAT, GL_FALSE, 0, 0);
    
    // Текстурные координаты
    glEnableVertexAttribArray(Attrib_texcoord);
    glBindBuffer(GL_ARRAY_BUFFER, TexCoordVBO_Center);
    glVertexAttribPointer(Attrib_texcoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    // Создаем временный буфер для одной матрицы
    GLuint tempInstanceVBO;
    glGenBuffers(1, &tempInstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, tempInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4), glm::value_ptr(modelMat), GL_STATIC_DRAW);
    
    // Настраиваем атрибуты для матрицы (4 vec4)
    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(2 + i);
        glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), 
                             (void*)(sizeof(glm::vec4) * i));
        glVertexAttribDivisor(2 + i, 1);
    }
    
    glDrawArraysInstanced(GL_TRIANGLES, 0, centerModel.vertexCount, 1);
    
    // Очистка
    for (int i = 0; i < 4; i++) {
        glDisableVertexAttribArray(2 + i);
    }
    glDeleteBuffers(1, &tempInstanceVBO);
    
    glDisableVertexAttribArray(Attrib_vertex);
    glDisableVertexAttribArray(Attrib_texcoord);
    glUseProgram(0);
    checkOpenGLerror();
}

void DrawOrbitingModels(float aspect) {
    if (!orbitLoaded || planets.empty()) return;
    
    glUseProgram(Program);
    
    // Матрицы преобразования
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 300.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    
    GLint view_loc = glGetUniformLocation(Program, "u_view");
    GLint proj_loc = glGetUniformLocation(Program, "u_projection");
    
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(projection));
    
    // Привязываем текстуру
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture_Orbit);
    GLint texture_loc = glGetUniformLocation(Program, "u_texture");
    glUniform1i(texture_loc, 0);
    
    // Обновляем буфер инстансов
    glBindBuffer(GL_ARRAY_BUFFER, InstanceVBO_Orbit);
    glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4), 
                 instanceMatrices.data(), GL_DYNAMIC_DRAW);
    
    // Вершины
    glEnableVertexAttribArray(Attrib_vertex);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Orbit);
    glVertexAttribPointer(Attrib_vertex, 3, GL_FLOAT, GL_FALSE, 0, 0);
    
    // Текстурные координаты
    glEnableVertexAttribArray(Attrib_texcoord);
    glBindBuffer(GL_ARRAY_BUFFER, TexCoordVBO_Orbit);
    glVertexAttribPointer(Attrib_texcoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    // Настраиваем инстанцированные атрибуты для матриц
    glBindBuffer(GL_ARRAY_BUFFER, InstanceVBO_Orbit);
    
    // Матрица 4x4 занимает 4 атрибута (каждый vec4)
    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(2 + i);
        glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), 
                             (void*)(sizeof(glm::vec4) * i));
        glVertexAttribDivisor(2 + i, 1); // Обновляем для каждого инстанса
    }
    
    // ИНСТАНЦИРОВАННЫЙ РЕНДЕРИНГ - один вызов для всех планет!
    glDrawArraysInstanced(GL_TRIANGLES, 0, orbitModel.vertexCount, planets.size());
    
    // Отключаем атрибуты
    for (int i = 0; i < 4; i++) {
        glDisableVertexAttribArray(2 + i);
    }
    
    glDisableVertexAttribArray(Attrib_vertex);
    glDisableVertexAttribArray(Attrib_texcoord);
    glUseProgram(0);
    checkOpenGLerror();
}

void ReleaseVBO() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (VBO_Center != 0) glDeleteBuffers(1, &VBO_Center);
    if (TexCoordVBO_Center != 0) glDeleteBuffers(1, &TexCoordVBO_Center);
    if (VBO_Orbit != 0) glDeleteBuffers(1, &VBO_Orbit);
    if (TexCoordVBO_Orbit != 0) glDeleteBuffers(1, &TexCoordVBO_Orbit);
    if (InstanceVBO_Orbit != 0) glDeleteBuffers(1, &InstanceVBO_Orbit);
}

void ReleaseShader() {
    glUseProgram(0);
    glDeleteProgram(Program);
}

void ReleaseTextures() {
    if (Texture_Center != 0) glDeleteTextures(1, &Texture_Center);
    if (Texture_Orbit != 0) glDeleteTextures(1, &Texture_Orbit);
}

void Release() {
    ReleaseTextures();
    ReleaseShader();
    ReleaseVBO();
}

void print_options() {
    std::cout << "\n=== Клавиши управления ===" << std::endl;
    std::cout << "1. 1 - загрузить центральную модель (christmas_house.obj + christmas_house_texture.jpg)" << std::endl;
    std::cout << "2. 2 - загрузить орбитальные модели (lolipop.obj + lolipop_texture.jpg)" << std::endl;
    std::cout << "3. ESC - выход" << std::endl;
    std::cout << "4. W/S - Вперед/Назад (Камера)" << std::endl;
    std::cout << "5. A/D - Влево/Вправо (Камера)" << std::endl;
    std::cout << "6. LShift/Space - Вниз/Вверх (Камера)" << std::endl;
    std::cout << "7. Мышь - Поворот камеры" << std::endl;
    std::cout << "\nИСПОЛЬЗУЕТСЯ ИНСТАНЦИРОВАННЫЙ РЕНДЕРИНГ!" << std::endl;
    std::cout << "Все орбитальные объекты отрисовываются за ОДИН вызов glDrawArraysInstanced()\n" << std::endl;
}

int main() {
    sf::Window window(sf::VideoMode(800, 600), "Solar System with Instancing", 
                     sf::Style::Default, sf::ContextSettings(24, 8, 0, 3, 3));
    window.setVerticalSyncEnabled(true);
    window.setMouseCursorVisible(false);
    window.setActive(true);
    
    glewInit();
    
    glEnable(GL_DEPTH_TEST);
    
    InitShader();
    InitSolarSystem();
    
    print_options();
    
    sf::Clock clock;

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::Resized) { 
                glViewport(0, 0, event.size.width, event.size.height); 
            }
            if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                    case sf::Keyboard::Escape: 
                        window.close(); 
                        break;
                    case sf::Keyboard::Num1:
                        LoadCenterModel("christmas_house.obj", "christmas_house_texture.jpg");
                        break;
                    case sf::Keyboard::Num2:
                        LoadOrbitModel("lolipop.obj", "lolipop_texture.jpg");
                        break;
                }
            }
            
            if (event.type == sf::Event::MouseMoved) {
                float xpos = static_cast<float>(event.mouseMove.x);
                float ypos = static_cast<float>(event.mouseMove.y);

                if (firstMouse) {
                    lastX = xpos;
                    lastY = ypos;
                    firstMouse = false;
                }

                float xoffset = xpos - lastX;
                float yoffset = lastY - ypos;
                lastX = xpos;
                lastY = ypos;

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
        
        float camSpeed = cameraSpeed * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
            cameraPos += camSpeed * cameraFront;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
            cameraPos -= camSpeed * cameraFront;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * camSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * camSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
            cameraPos += camSpeed * cameraUp;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
            cameraPos -= camSpeed * cameraUp;

        UpdatePlanets(deltaTime);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.05f, 1.0f);

        float aspect = (float)window.getSize().x / (float)window.getSize().y;
        
        DrawCenterModel(aspect);
        DrawOrbitingModels(aspect);

        window.display();
    }
    
    Release();
    return 0;
}
