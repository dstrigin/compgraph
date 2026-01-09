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

// ID шейдерной программы (Основная)
GLuint Program;
// ID шейдерной программы (Для лампочки - чтобы она светилась сама)
GLuint LampProgram;

// ID атрибутов шейдера
GLint Attrib_pos;
GLint Attrib_tex;
GLint Attrib_norm;

// Uniforms для матриц
GLint Unif_model;
GLint Unif_view;
GLint Unif_proj;
GLint Unif_viewPos;
GLint Unif_materialShininess;

// Uniform для выбора модели освещения (0 - Phong, 1 - Toon, 2 - Minnaert)
GLint Unif_lightingModel;

// --- Состояние приложения ---
enum ControlMode {
    MODE_CAMERA = 0,    // Управление полетом камеры
    MODE_POINTLIGHT,    // Управление положением и яркостью лампочки
    MODE_SPOTLIGHT      // Управление шириной конуса и яркостью фонарика
};
ControlMode currentMode = MODE_CAMERA;

// Переменные точечного источника (изменяемые)
glm::vec3 pointLightPos = glm::vec3(-4.0f, 2.0f, 10.0f);
float pointLightIntensity = 1.0f; 

// Переменные прожектора (изменяемые)
bool spotLightOn = true;        // Вкл/Выкл по клавише L
float spotLightCutOffAngle = 12.5f; // Угол конуса
float spotLightIntensity = 1.0f;

// Объект визуализации лампочки
GLuint LightCubeVAO, LightCubeVBO;

// --- Структуры для Uniforms освещения ---

// Направленный свет (Солнце)
struct DirLight {
    GLint direction;
    GLint ambient;
    GLint diffuse;
    GLint specular;
} dirLightLoc;

// Точечный источник (Лампочка)
struct PointLight {
    GLint position;
    GLint constant;
    GLint linear;
    GLint quadratic;
    GLint ambient;
    GLint diffuse;
    GLint specular;
} pointLightLoc;

// Прожектор (Фонарик)
struct SpotLight {
    GLint position;
    GLint direction;
    GLint cutOff;
    GLint outerCutOff;
    GLint constant;
    GLint linear;
    GLint quadratic;
    GLint ambient;
    GLint diffuse;
    GLint specular;
} spotLightLoc;

// Камера
glm::vec3 cameraPos   = glm::vec3(0.0f, 2.0f, 10.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw    = -90.0f;
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
    
    // Тип освещения: 0 - Phong, 1 - Toon, 2 - Minnaert
    int lightingType;
};

std::vector<GameObject> sceneObjects;

// --- Шейдеры (Phong + Toon + Minnaert) ---

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

uniform sampler2D texture1;
uniform vec3 viewPos;
uniform float materialShininess;

// Выбор модели: 0 = Phong, 1 = Toon, 2 = Minnaert
uniform int u_lightingModel; 

// --- Структуры источников света ---

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    
    float cutOff;
    float outerCutOff;
    
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform DirLight dirLight;
uniform PointLight pointLight;
uniform SpotLight spotLight;

// --- Прототипы функций расчета освещения ---
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main() {
    // Свойства материала и нормаль
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // 1. Направленный источник (Солнце)
    vec3 result = CalcDirLight(dirLight, norm, viewDir);
    
    // 2. Точечный источник (Лампочка)
    result += CalcPointLight(pointLight, norm, FragPos, viewDir);
    
    // 3. Прожектор (Фонарик)
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir);
    
    FragColor = vec4(result, 1.0);
}

// Расчет направленного света
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    
    // Базовые вычисления
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    
    // --- Модификация по моделям ---
    if (u_lightingModel == 1) { // Toon Shading
        // Дискретизация диффузного света
        float intensity = dot(normal, lightDir);
        if (intensity > 0.95) diff = 1.0;
        else if (intensity > 0.5) diff = 0.6;
        else if (intensity > 0.25) diff = 0.3;
        else diff = 0.1;
        
        // Резкий блик
        if (spec > 0.5) spec = 1.0;
        else spec = 0.0;
    }
    else if (u_lightingModel == 2) { // Minnaert
        float k = 0.8;
        // Формула: d1 * d2
        float d1 = pow(max(dot(normal, lightDir), 0.0), 1.0 + k);
        float ndotv = max(dot(normal, viewDir), 0.0); // Защита от отр. значений
        float d2 = pow(1.0 - ndotv, 1.0 - k);
        
        diff = d1 * d2;
        // В Minnaert спекуляр часто слабый или стандартный, оставим стандартный но ослабленный
        spec *= 0.5;
    }
    // else == 0 -> Phong (оставляем как есть)

    vec3 ambient  = light.ambient  * vec3(texture(texture1, TexCoord));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(texture1, TexCoord));
    vec3 specular = light.specular * spec; 
    return (ambient + diffuse + specular);
}

// Расчет точечного источника
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    
    // --- Модификация по моделям ---
    if (u_lightingModel == 1) { // Toon
        float intensity = dot(normal, lightDir);
        if (intensity > 0.95) diff = 1.0;
        else if (intensity > 0.5) diff = 0.6;
        else if (intensity > 0.25) diff = 0.3;
        else diff = 0.1;

        if (spec > 0.5) spec = 1.0; else spec = 0.0;
    }
    else if (u_lightingModel == 2) { // Minnaert
        float k = 0.8;
        float d1 = pow(max(dot(normal, lightDir), 0.0), 1.0 + k);
        float ndotv = max(dot(normal, viewDir), 0.0);
        float d2 = pow(1.0 - ndotv, 1.0 - k);
        diff = d1 * d2;
        spec *= 0.5;
    }

    // Attenuation (Затухание)
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));     
    
    vec3 ambient  = light.ambient  * vec3(texture(texture1, TexCoord));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(texture1, TexCoord));
    vec3 specular = light.specular * spec;
    
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

// Расчет прожектора
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    
    // --- Модификация по моделям ---
    if (u_lightingModel == 1) { // Toon
        float intensity = dot(normal, lightDir);
        if (intensity > 0.95) diff = 1.0;
        else if (intensity > 0.5) diff = 0.6;
        else if (intensity > 0.25) diff = 0.3;
        else diff = 0.1;

        if (spec > 0.5) spec = 1.0; else spec = 0.0;
    }
    else if (u_lightingModel == 2) { // Minnaert
        float k = 0.8;
        float d1 = pow(max(dot(normal, lightDir), 0.0), 1.0 + k);
        float ndotv = max(dot(normal, viewDir), 0.0);
        float d2 = pow(1.0 - ndotv, 1.0 - k);
        diff = d1 * d2;
        spec *= 0.5;
    }

    // Attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));     
    
    // Spotlight intensity (Мягкие края)
    float theta     = dot(lightDir, normalize(-light.direction)); 
    float epsilon   = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    // Для Toon шейдинга можно убрать мягкие края прожектора (сделать резкими),
    // но для простоты оставим intensity как есть.

    vec3 ambient  = light.ambient  * vec3(texture(texture1, TexCoord));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(texture1, TexCoord));
    vec3 specular = light.specular * spec;
    
    ambient  *= attenuation * intensity; 
    diffuse  *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return (ambient + diffuse + specular);
}
)";

// --- Простой шейдер для отображения источника света (Лампы) ---
const char* LampVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* LampFragShaderSource = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0); // Всегда белый цвет
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
    // --- Основной шейдер ---
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &VertexShaderSource, NULL);
    glCompileShader(vShader);
    
    GLint success; char infoLog[512];
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
    if(!success) { glGetShaderInfoLog(vShader, 512, NULL, infoLog); std::cout << "VS Error: " << infoLog << std::endl; }

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &FragShaderSource, NULL);
    glCompileShader(fShader);
    
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
    
    Unif_viewPos        = glGetUniformLocation(Program, "viewPos");
    Unif_materialShininess = glGetUniformLocation(Program, "materialShininess");
    
    // Получаем локацию для выбора модели освещения
    Unif_lightingModel = glGetUniformLocation(Program, "u_lightingModel");

    // --- Локации для Направленного света (Directional) ---
    dirLightLoc.direction = glGetUniformLocation(Program, "dirLight.direction");
    dirLightLoc.ambient   = glGetUniformLocation(Program, "dirLight.ambient");
    dirLightLoc.diffuse   = glGetUniformLocation(Program, "dirLight.diffuse");
    dirLightLoc.specular  = glGetUniformLocation(Program, "dirLight.specular");

    // --- Локации для Точечного света (Point) ---
    pointLightLoc.position  = glGetUniformLocation(Program, "pointLight.position");
    pointLightLoc.ambient   = glGetUniformLocation(Program, "pointLight.ambient");
    pointLightLoc.diffuse   = glGetUniformLocation(Program, "pointLight.diffuse");
    pointLightLoc.specular  = glGetUniformLocation(Program, "pointLight.specular");
    pointLightLoc.constant  = glGetUniformLocation(Program, "pointLight.constant");
    pointLightLoc.linear    = glGetUniformLocation(Program, "pointLight.linear");
    pointLightLoc.quadratic = glGetUniformLocation(Program, "pointLight.quadratic");

    // --- Локации для Прожектора (Spot) ---
    spotLightLoc.position    = glGetUniformLocation(Program, "spotLight.position");
    spotLightLoc.direction   = glGetUniformLocation(Program, "spotLight.direction");
    spotLightLoc.cutOff      = glGetUniformLocation(Program, "spotLight.cutOff");
    spotLightLoc.outerCutOff = glGetUniformLocation(Program, "spotLight.outerCutOff");
    spotLightLoc.ambient     = glGetUniformLocation(Program, "spotLight.ambient");
    spotLightLoc.diffuse     = glGetUniformLocation(Program, "spotLight.diffuse");
    spotLightLoc.specular    = glGetUniformLocation(Program, "spotLight.specular");
    spotLightLoc.constant    = glGetUniformLocation(Program, "spotLight.constant");
    spotLightLoc.linear      = glGetUniformLocation(Program, "spotLight.linear");
    spotLightLoc.quadratic   = glGetUniformLocation(Program, "spotLight.quadratic");

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    // --- Шейдер для Лампы (Маркера) ---
    GLuint vLamp = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vLamp, 1, &LampVertexShaderSource, NULL);
    glCompileShader(vLamp);
    GLuint fLamp = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fLamp, 1, &LampFragShaderSource, NULL);
    glCompileShader(fLamp);
    LampProgram = glCreateProgram();
    glAttachShader(LampProgram, vLamp);
    glAttachShader(LampProgram, fLamp);
    glLinkProgram(LampProgram);
    glDeleteShader(vLamp);
    glDeleteShader(fLamp);
}

// Инициализация кубика для визуализации света
void InitLightMarker() {
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,

         0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f
    };

    glGenVertexArrays(1, &LightCubeVAO);
    glGenBuffers(1, &LightCubeVBO);
    glBindVertexArray(LightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, LightCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// Добавление объекта в сцену
void AddObject(const char* objPath, const char* texPath, glm::vec3 pos, glm::vec3 scale, glm::vec3 rot, std::string name, int lightType = 0) {
    ModelData data;
    if (LoadOBJ(objPath, data)) {
        GameObject obj;
        obj.vertexCount = data.vertexCount;
        obj.position = pos;
        obj.scale = scale;
        obj.rotation = rot;
        obj.name = name;
        obj.lightingType = lightType; // Сохраняем тип освещения

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
    InitLightMarker(); // Инициализация кубика света

    // 1. Домик - PHONG (Стандартный)
    AddObject("assets/house.obj", "assets/house.jpg", 
              glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.1f), glm::vec3(0.0f, 0.0f, 0.0f), "House", 0);

    // 2. Сани - PHONG (Дерево)
    AddObject("assets/sleigh.obj", "assets/sleigh.jpg", 
              glm::vec3(-10.0f, -1.0f, 1.0f), glm::vec3(0.001f), glm::vec3(0.0f, -45.0f, 0.0f), "Sleigh", 0);

    // 3. Воздушный шар - MINNAERT (Ткань/Резина)
    // Модель Minnaert хорошо подчеркивает бархатистость и объем на сферических объектах из ткани
    AddObject("assets/balloon.obj", "assets/balloon.jpg", 
              glm::vec3(-5.0f, 5.0f, -2.0f), glm::vec3(0.01f), glm::vec3(0.0f, 0.0f, 0.0f), "Balloon", 2);

    // 4. Леденец - TOON SHADING (Мультяшная конфета)
    // Яркая стилизованная модель отлично подходит для сладостей
    AddObject("assets/lollipop.obj", "assets/lollipop.jpg", 
              glm::vec3(-10.0f, -0.5f, 1.0f), glm::vec3(0.05f), glm::vec3(0.0f, 30.0f, 0.0f), "Lollipop", 1);

    // 5. Дирижабль - MINNAERT (Ткань корпуса)
    AddObject("assets/zeppelin.obj", "assets/zeppelin.jpg", 
              glm::vec3(4.0f, 25.0f, -3.0f), glm::vec3(4.0f), glm::vec3(0.0f, 90.0f, 0.0f), "Zeppelin", 2);
}

void DrawScene(float aspect) {
    // --- Матрицы камеры ---
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    // --- Рисуем объекты сцены с основным шейдером ---
    glUseProgram(Program);

    // --- Настройка освещения ---

    // 1. Направленный свет (Солнце) - Статический
    glUniform3f(dirLightLoc.direction, -0.5f, -1.0f, -0.3f);
    glUniform3f(dirLightLoc.ambient,   0.05f, 0.05f, 0.05f); 
    glUniform3f(dirLightLoc.diffuse,   0.4f, 0.4f, 0.4f);    
    glUniform3f(dirLightLoc.specular,  0.5f, 0.5f, 0.5f);

    // 2. Точечный свет (Лампочка) 
    glUniform3fv(pointLightLoc.position, 1, glm::value_ptr(pointLightPos)); 
    glUniform3f(pointLightLoc.ambient,   0.05f, 0.05f, 0.05f);
    // Применяем интенсивность к цвету лампы (красный оттенок)
    glUniform3f(pointLightLoc.diffuse,   0.8f * pointLightIntensity, 0.1f * pointLightIntensity, 0.1f * pointLightIntensity);  
    glUniform3f(pointLightLoc.specular,  1.0f * pointLightIntensity, 0.5f * pointLightIntensity, 0.5f * pointLightIntensity);
    glUniform1f(pointLightLoc.constant,  1.0f);
    glUniform1f(pointLightLoc.linear,    0.09f);
    glUniform1f(pointLightLoc.quadratic, 0.032f);

    // 3. Прожектор 
    glUniform3fv(spotLightLoc.position,  1, glm::value_ptr(cameraPos));
    glUniform3fv(spotLightLoc.direction, 1, glm::value_ptr(cameraFront));
    glUniform3f(spotLightLoc.ambient,    0.0f, 0.0f, 0.0f);

    if (spotLightOn) {
        // Если включен, применяем интенсивность
        glUniform3f(spotLightLoc.diffuse,    1.0f * spotLightIntensity, 1.0f * spotLightIntensity, 1.0f * spotLightIntensity);  
        glUniform3f(spotLightLoc.specular,   1.0f * spotLightIntensity, 1.0f * spotLightIntensity, 1.0f * spotLightIntensity);
    } else {
        // Если выключен - черный свет
        glUniform3f(spotLightLoc.diffuse,    0.0f, 0.0f, 0.0f);  
        glUniform3f(spotLightLoc.specular,   0.0f, 0.0f, 0.0f);
    }

    glUniform1f(spotLightLoc.constant,   1.0f);
    glUniform1f(spotLightLoc.linear,     0.09f);
    glUniform1f(spotLightLoc.quadratic,  0.032f);
    
    // Конус света (динамический cutOff)
    // outerCutOff сделаем чуть шире основного для мягкости
    glUniform1f(spotLightLoc.cutOff,      glm::cos(glm::radians(spotLightCutOffAngle))); 
    glUniform1f(spotLightLoc.outerCutOff, glm::cos(glm::radians(spotLightCutOffAngle + 5.0f))); 

    glUniform3fv(Unif_viewPos, 1, glm::value_ptr(cameraPos));
    glUniform1f(Unif_materialShininess, 32.0f); 

    glUniformMatrix4fv(Unif_view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(Unif_proj, 1, GL_FALSE, glm::value_ptr(projection));

    // --- Отрисовка всех объектов ---
    for (const auto& obj : sceneObjects) {
        // Передаем тип освещения для текущего объекта
        glUniform1i(Unif_lightingModel, obj.lightingType);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, obj.position);
        model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1,0,0));
        model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0,1,0));
        model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0,0,1));
        model = glm::scale(model, obj.scale);

        glUniformMatrix4fv(Unif_model, 1, GL_FALSE, glm::value_ptr(model));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, obj.TextureID);

        glBindVertexArray(obj.VAO);
        glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount);
        glBindVertexArray(0);
    }

    // --- Рисуем маркер источника света (Лампочка) ---
    // Используем отдельный простой шейдер
    glUseProgram(LampProgram);
    
    // Передаем матрицы в Lamp Shader
    GLint lampModelLoc = glGetUniformLocation(LampProgram, "model");
    GLint lampViewLoc  = glGetUniformLocation(LampProgram, "view");
    GLint lampProjLoc  = glGetUniformLocation(LampProgram, "projection");

    glm::mat4 lampModel = glm::mat4(1.0f);
    lampModel = glm::translate(lampModel, pointLightPos);
    lampModel = glm::scale(lampModel, glm::vec3(0.2f)); // Маленький кубик

    glUniformMatrix4fv(lampModelLoc, 1, GL_FALSE, glm::value_ptr(lampModel));
    glUniformMatrix4fv(lampViewLoc,  1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(lampProjLoc,  1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(LightCubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

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
    glDeleteVertexArrays(1, &LightCubeVAO);
    glDeleteBuffers(1, &LightCubeVBO);
    glDeleteProgram(Program);
    glDeleteProgram(LampProgram);
}

int main() {
    sf::Window window(sf::VideoMode(800, 600), "Multiple Lights + Shading Models", 
                      sf::Style::Default, sf::ContextSettings(24, 8, 0, 3, 3));
    window.setVerticalSyncEnabled(true);
    window.setMouseCursorVisible(false);
    window.setActive(true);
    
    glewInit();
    glEnable(GL_DEPTH_TEST);
    
    InitShader();
    InitScene(); // Загружает объекты и маркер света
    
    std::cout << "Scene Loaded." << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "[TAB]   Switch Control Mode (Camera -> Point Light -> Spot Light)" << std::endl;
    std::cout << "[L]     Toggle Flashlight (Spotlight) ON/OFF" << std::endl;
    std::cout << "--- Mode: CAMERA ---" << std::endl;
    std::cout << "WASD: Move, Mouse: Look, Shift/Space: Up/Down" << std::endl;
    std::cout << "--- Mode: POINT LIGHT (Bulb) ---" << std::endl;
    std::cout << "Arrows/PgUp/PgDn: Move Light Position" << std::endl;
    std::cout << "+/- : Adjust Intensity" << std::endl;
    std::cout << "--- Mode: SPOT LIGHT (Flashlight) ---" << std::endl;
    std::cout << "Left/Right Arrows: Adjust Cone Width" << std::endl;
    std::cout << "+/- : Adjust Intensity" << std::endl;

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

            // Переключение режимов по TAB
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Tab) {
                if (currentMode == MODE_CAMERA) {
                    currentMode = MODE_POINTLIGHT;
                    std::cout << "Mode switched to: POINT LIGHT EDIT" << std::endl;
                } else if (currentMode == MODE_POINTLIGHT) {
                    currentMode = MODE_SPOTLIGHT;
                    std::cout << "Mode switched to: SPOT LIGHT EDIT" << std::endl;
                } else {
                    currentMode = MODE_CAMERA;
                    std::cout << "Mode switched to: CAMERA FLY" << std::endl;
                }
            }

            // Включение/Выключение прожектора (L) - работает всегда
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::L) {
                spotLightOn = !spotLightOn;
                std::cout << "Spotlight: " << (spotLightOn ? "ON" : "OFF") << std::endl;
            }
            
            // Обработка мыши (Работает только в режиме камеры)
            if (currentMode == MODE_CAMERA && event.type == sf::Event::MouseMoved) {
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
            // Обновляем позицию мыши даже если не двигаем камеру, чтобы не было скачка при возврате
            if (currentMode != MODE_CAMERA && event.type == sf::Event::MouseMoved) {
                 lastX = static_cast<float>(event.mouseMove.x);
                 lastY = static_cast<float>(event.mouseMove.y);
            }
        }
        
        // --- Управление в зависимости от режима ---

        if (currentMode == MODE_CAMERA) {
            // Управление камерой
            float camSpeed = cameraSpeed * deltaTime;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) cameraPos += camSpeed * cameraFront;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) cameraPos -= camSpeed * cameraFront;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * camSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * camSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) cameraPos += camSpeed * cameraUp;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) cameraPos -= camSpeed * cameraUp;
        }
        else if (currentMode == MODE_POINTLIGHT) {
            // Настройка точечного источника
            float lightMoveSpeed = 5.0f * deltaTime;
            // Перемещение (Стрелки - X/Z, PageUp/Down - Y)
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    pointLightPos.z -= lightMoveSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  pointLightPos.z += lightMoveSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  pointLightPos.x -= lightMoveSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) pointLightPos.x += lightMoveSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp))   pointLightPos.y += lightMoveSpeed;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown)) pointLightPos.y -= lightMoveSpeed;

            // Интенсивность (+/-)
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Add) || sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) 
                pointLightIntensity += 1.0f * deltaTime;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Subtract) || sf::Keyboard::isKeyPressed(sf::Keyboard::Hyphen)) 
                pointLightIntensity -= 1.0f * deltaTime;
            if (pointLightIntensity < 0.0f) pointLightIntensity = 0.0f;
        }
        else if (currentMode == MODE_SPOTLIGHT) {
            // Настройка прожектора
            // Ширина конуса (Стрелки влево/вправо)
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) spotLightCutOffAngle += 10.0f * deltaTime;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  spotLightCutOffAngle -= 10.0f * deltaTime;
            
            // Ограничиваем угол (от 1 до 90 градусов)
            if (spotLightCutOffAngle < 1.0f) spotLightCutOffAngle = 1.0f;
            if (spotLightCutOffAngle > 90.0f) spotLightCutOffAngle = 90.0f;

            // Интенсивность (+/-)
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Add) || sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) 
                spotLightIntensity += 1.0f * deltaTime;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Subtract) || sf::Keyboard::isKeyPressed(sf::Keyboard::Hyphen)) 
                spotLightIntensity -= 1.0f * deltaTime;
            if (spotLightIntensity < 0.0f) spotLightIntensity = 0.0f;
        }

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
