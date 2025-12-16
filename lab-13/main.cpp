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
// ID вершинного атрибута
GLint Attrib_vertex;
// ID атрибута текстурных координат
GLint Attrib_texcoord;
// ID Vertex Buffer Object для центральной модели
GLuint VBO_Center;
GLuint TexCoordVBO_Center;
GLuint Texture_Center;
// ID Vertex Buffer Object для орбитальных моделей
GLuint VBO_Orbit;
GLuint TexCoordVBO_Orbit;
GLuint Texture_Orbit;

// камера
glm::vec3 cameraPos   = glm::vec3(0.0f, 5.0f, 20.0f); 
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw   = -90.0f;	// влево-вправо
float pitch = -10.0f;	// вверх-вниз
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

bool centerLoaded = false;
bool orbitLoaded = false;

// Исходный код вершинного шейдера
const char* VertexShaderSource = R"(
#version 330 core
in vec3 coord;
in vec2 texcoord;

out vec2 v_texcoord;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main() {
    gl_Position = u_projection * u_view * u_model * vec4(coord, 1.0);
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
            // Вершина
            Vertex v;
            iss >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        }
        else if (prefix == "vt") {
            // Текстурные координаты
            TexCoord tc;
            iss >> tc.u >> tc.v;
            temp_texcoords.push_back(tc);
        }
        else if (prefix == "f") {
            // Грань
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
                // Если нет текстурных координат, генерируем их
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
    
    const char* attr_name = "coord";
    Attrib_vertex = glGetAttribLocation(Program, attr_name);
    if (Attrib_vertex == -1) {
        std::cout << "could not bind attrib " << attr_name << std::endl;
        return;
    }
    
    const char* texcoord_name = "texcoord";
    Attrib_texcoord = glGetAttribLocation(Program, texcoord_name);
    if (Attrib_texcoord == -1) {
        std::cout << "could not bind attrib " << texcoord_name << std::endl;
        return;
    }
    
    checkOpenGLerror();
}

void InitSolarSystem() {
    planets.clear();
    
    float customSizes[] = {0.15f, 0.55f, 0.3f, 0.80f, 1.2f};
    for(int i = 0; i < 5; i++) {
        Planet p;
        p.orbitRadius = 12.0f;      
        p.orbitSpeed = 0.8f;       
        p.rotationSpeed = 1.5f;     
        p.size = customSizes[i];            
        p.orbitAngle = (M_PI * 2 / 5) * i; 
        p.rotationAngle = 0.0f;
        planets.push_back(p);
    }
    
    std::cout << "Solar system initialized with " << planets.size() << " orbiting objects" << std::endl;
}

void LoadCenterModel(const char* objFile, const char* textureFile) {
    std::cout << "Loading center model..." << std::endl;
    if (LoadOBJ(objFile, centerModel)) {
        // Создаем VBO 
        glGenBuffers(1, &VBO_Center);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_Center);
        glBufferData(GL_ARRAY_BUFFER, centerModel.vertices.size() * sizeof(Vertex), 
                     centerModel.vertices.data(), GL_STATIC_DRAW); //чисто чтение GPU
        
        glGenBuffers(1, &TexCoordVBO_Center);
        glBindBuffer(GL_ARRAY_BUFFER, TexCoordVBO_Center);
        glBufferData(GL_ARRAY_BUFFER, centerModel.texcoords.size() * sizeof(TexCoord), 
                     centerModel.texcoords.data(), GL_STATIC_DRAW);
        
        // Загружаем текстуру
        Texture_Center = LoadTexture(textureFile);
        
        centerLoaded = true;
        std::cout << "Center model loaded successfully!" << std::endl;
        checkOpenGLerror();
    }
}

void LoadOrbitModel(const char* objFile, const char* textureFile) {
    std::cout << "Loading orbit model..." << std::endl;
    if (LoadOBJ(objFile, orbitModel)) {
        // Создаем VBO 
        glGenBuffers(1, &VBO_Orbit);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_Orbit);
        glBufferData(GL_ARRAY_BUFFER, orbitModel.vertices.size() * sizeof(Vertex), 
                     orbitModel.vertices.data(), GL_STATIC_DRAW); //копируем всю модельку в gpu
        
        glGenBuffers(1, &TexCoordVBO_Orbit);
        glBindBuffer(GL_ARRAY_BUFFER, TexCoordVBO_Orbit);
        glBufferData(GL_ARRAY_BUFFER, orbitModel.texcoords.size() * sizeof(TexCoord), 
                     orbitModel.texcoords.data(), GL_STATIC_DRAW);
        
        // Загружаем текстуру
        Texture_Orbit = LoadTexture(textureFile);
        
        orbitLoaded = true;
        std::cout << "Orbit model loaded successfully!" << std::endl;
        checkOpenGLerror();
    }
}

void UpdatePlanets(float deltaTime) {
    for (size_t i = 0; i < planets.size(); i++) {
        // угол на орбите
        planets[i].orbitAngle += planets[i].orbitSpeed * deltaTime;
        
        // угол поворота вокруг своей оси
        planets[i].rotationAngle += planets[i].rotationSpeed * deltaTime;
    }
}

void DrawModel(const ModelData& model, GLuint vbo, GLuint texVbo, GLuint texture,
               float x, float y, float z, float size, float rotationAngle, float aspect) {
    glUseProgram(Program); 
    
    // Матрицы преобразования
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 300.0f); 
    
    //Используем динамическую камеру 
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    
    glm::mat4 modelMat = glm::mat4(1.0f);
    modelMat = glm::translate(modelMat, glm::vec3(x, y, z));
    modelMat = glm::rotate(modelMat, rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::scale(modelMat, glm::vec3(size));
    
    // Передаем uniform-переменные
    GLint model_loc = glGetUniformLocation(Program, "u_model");
    GLint view_loc = glGetUniformLocation(Program, "u_view");
    GLint proj_loc = glGetUniformLocation(Program, "u_projection");
    
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(projection));
    
    // Привязываем текстуру
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    GLint texture_loc = glGetUniformLocation(Program, "u_texture");
    glUniform1i(texture_loc, 0);
    
    // Включаем массив атрибутов для вершин
    glEnableVertexAttribArray(Attrib_vertex);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(Attrib_vertex, 3, GL_FLOAT, GL_FALSE, 0, 0);
    
    // Включаем массив атрибутов для текстурных координат
    glEnableVertexAttribArray(Attrib_texcoord);
    glBindBuffer(GL_ARRAY_BUFFER, texVbo);
    glVertexAttribPointer(Attrib_texcoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Отключаем VBO
    
    // Рисуем модель
    glDrawArrays(GL_TRIANGLES, 0, model.vertexCount);
    
    glDisableVertexAttribArray(Attrib_vertex); // Отключаем массив атрибутов
    glDisableVertexAttribArray(Attrib_texcoord);
    glUseProgram(0); // Отключаем шейдерную программу
    checkOpenGLerror();
}

void DrawCenterModel(float aspect) {
    if (!centerLoaded) return;
    
    static float rotationAngle = 0.0f;
    rotationAngle += 0.05f * 0.016f; 
    
    DrawModel(centerModel, VBO_Center, TexCoordVBO_Center, Texture_Center,
              0.0f, 0.0f, 0.0f, 0.05f, rotationAngle, aspect);
}

void DrawOrbitingModels(float aspect) {
    if (!orbitLoaded) return;
    
    for (size_t i = 0; i < planets.size(); i++) {
        float x = planets[i].orbitRadius * cos(planets[i].orbitAngle);
        float z = planets[i].orbitRadius * sin(planets[i].orbitAngle);
        float y = -20.65f * planets[i].size;
        
        DrawModel(orbitModel, VBO_Orbit, TexCoordVBO_Orbit, Texture_Orbit,
                  x, y, z, planets[i].size, planets[i].rotationAngle, aspect);
    }
}

// Освобождение буфера
void ReleaseVBO() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (VBO_Center != 0) glDeleteBuffers(1, &VBO_Center);
    if (TexCoordVBO_Center != 0) glDeleteBuffers(1, &TexCoordVBO_Center);
    if (VBO_Orbit != 0) glDeleteBuffers(1, &VBO_Orbit);
    if (TexCoordVBO_Orbit != 0) glDeleteBuffers(1, &TexCoordVBO_Orbit);
}

// Освобождение шейдеров
void ReleaseShader() {
    // отключаем шейдерную программу
    glUseProgram(0);
    // Удаляем шейдерную программу
    glDeleteProgram(Program);
}

// Освобождение текстур
void ReleaseTextures() {
    if (Texture_Center != 0) glDeleteTextures(1, &Texture_Center);
    if (Texture_Orbit != 0) glDeleteTextures(1, &Texture_Orbit);
}

void Release() {
    // Текстуры
    ReleaseTextures();
    // Шейдеры
    ReleaseShader();
    // Вершинный буфер
    ReleaseVBO();
}

void print_options() {
    std::cout << "Клавиши управления:" << std::endl;
    std::cout << "1. 1 - загрузить центральную модель (model1.obj + texture1.jpg)" << std::endl;
    std::cout << "2. 2 - загрузить орбитальные модели (model2.obj + texture2.jpg)" << std::endl;
    std::cout << "3. Space - запустить/остановить анимацию" << std::endl;
    std::cout << "4. ESC - выход" << std::endl;
    std::cout << "5. W/S - Вперед/Назад (Камера)" << std::endl;
    std::cout << "6. A/D - Влево/Вправо (Камера)" << std::endl;
    std::cout << "7. LShift/Space(в режиме полета) - Вниз/Вверх (Камера)" << std::endl;
    std::cout << "8. Мышь - Поворот камеры" << std::endl;
}

int main() {
    sf::Window window(sf::VideoMode(800, 600), "Solar System", 
                     sf::Style::Default, sf::ContextSettings(24, 8, 0, 3, 3));
    window.setVerticalSyncEnabled(true);
    window.setMouseCursorVisible(false); // Скрываем курсор для управления мышью
    window.setActive(true);
    
    glewInit();
    
    // Включаем тест глубины
    glEnable(GL_DEPTH_TEST);
    
    InitShader();
    InitSolarSystem();
    
    bool animating = true;
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
            
            //мышка
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
        
        // клава
        float camSpeed = cameraSpeed * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
            cameraPos += camSpeed * cameraFront;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
            cameraPos -= camSpeed * cameraFront;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * camSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * camSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) // Вверх
            cameraPos += camSpeed * cameraUp;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) // Вниз
            cameraPos -= camSpeed * cameraUp;

        // Обновляем планеты
        if (animating) {
            UpdatePlanets(deltaTime);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.05f, 1.0f);

        float aspect = (float)window.getSize().x / (float)window.getSize().y;
        
        // Рисуем центральную модель
        DrawCenterModel(aspect);
        
        // Рисуем орбитальные модели
        DrawOrbitingModels(aspect);

        window.display();
    }
    
    Release();
    return 0;
}
