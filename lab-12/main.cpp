#include <iostream>
#include <vector>
#include <cmath>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Перечисление для выбора задания
enum Assignment {
    ASSIGNMENT_1 = 1, // Градиентный тетраэдр
    ASSIGNMENT_2 = 2, // Кубик с текстурой и цветом
    ASSIGNMENT_3 = 3, // Кубик с двумя текстурами
    ASSIGNMENT_4 = 4  // Градиентный круг
};

Assignment currentAssignment = ASSIGNMENT_1;

// Структуры для хранения данных вершин
struct Vertex3DWithColor {
    GLfloat x, y, z;    // координаты
    GLfloat r, g, b, a; // цвет с альфа-каналом
};

struct Vertex3DWithTex {
    GLfloat x, y, z;    // координаты
    GLfloat u, v;       // текстурные координаты
};

// Вспомогательная функция для вывода логов шейдеров
void ShaderLog(unsigned int shader) {
    int logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1) {
        std::vector<char> log(logLength);
        glGetShaderInfoLog(shader, logLength, NULL, log.data());
        std::cout << "Лог шейдера: " << log.data() << std::endl;
    }
}

// Проверка ошибок OpenGL
void checkOpenGLerror() {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "Ошибка OpenGL: " << err << std::endl;
    }
}

// ========================== ЗАДАНИЕ 1: ГРАДИЕНТНЫЙ ТЕТРАЭДР ==========================

GLuint VBO_Tetrahedron, ProgramTetrahedron;
glm::vec3 tetrahedronPos(0.0f);
GLuint Attrib_vertex_tetra, Attrib_color_tetra;
GLuint Uniform_offset_tetra, Uniform_projection_tetra, Uniform_view_tetra, Uniform_model_tetra;

// Вершинный шейдер для тетраэдра
const char* VertexShaderTetra = R"(
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 offset;
out vec4 vertexColor;
void main() {
    vec3 rotatedPosition = (model * vec4(position, 1.0)).xyz;
    vec3 worldPosition = rotatedPosition + offset;
    gl_Position = projection * view * vec4(worldPosition, 1.0);
    vertexColor = color;
}
)";

// Фрагментный шейдер для тетраэдра
const char* FragShaderTetra = R"(
#version 330 core
in vec4 vertexColor;
out vec4 FragColor;
void main() {
    FragColor = vertexColor;
}
)";

// Инициализация шейдеров для тетраэдра
void Tetrahedron_InitShader() {
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &VertexShaderTetra, NULL);
    glCompileShader(vShader);
    ShaderLog(vShader);

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &FragShaderTetra, NULL);
    glCompileShader(fShader);
    ShaderLog(fShader);

    ProgramTetrahedron = glCreateProgram();
    glAttachShader(ProgramTetrahedron, vShader);
    glAttachShader(ProgramTetrahedron, fShader);
    glLinkProgram(ProgramTetrahedron);

    Attrib_vertex_tetra = glGetAttribLocation(ProgramTetrahedron, "position");
    Attrib_color_tetra = glGetAttribLocation(ProgramTetrahedron, "color");
    Uniform_offset_tetra = glGetUniformLocation(ProgramTetrahedron, "offset");
    Uniform_projection_tetra = glGetUniformLocation(ProgramTetrahedron, "projection");
    Uniform_view_tetra = glGetUniformLocation(ProgramTetrahedron, "view");
    Uniform_model_tetra = glGetUniformLocation(ProgramTetrahedron, "model");

    glDeleteShader(vShader);
    glDeleteShader(fShader);
    checkOpenGLerror();
}

// Создание вершин тетраэдра с улучшенными градиентами (как у ребят)
std::vector<Vertex3DWithColor> createTetrahedron() {
    std::vector<Vertex3DWithColor> vertices;
    float h = 0.8f;  // высота тетраэдра
    float r = 0.6f;  // радиус описанной окружности основания
    
    // Верхняя вершина (белая)
    float top_x = 0.0f, top_y = h, top_z = 0.0f;
    
    // Три вершины основания
    float base_y = -0.3f;
    float v1_x = r, v1_y = base_y, v1_z = 0.0f;                    
    float v2_x = -r/2.0f, v2_y = base_y, v2_z = r*0.866f;          
    float v3_x = -r/2.0f, v3_y = base_y, v3_z = -r*0.866f;        

    // Грань 1: белая, зеленая, синяя
    vertices.push_back({top_x, top_y, top_z, 1.0f, 1.0f, 1.0f, 1.0f}); // белая
    vertices.push_back({v1_x, v1_y, v1_z, 0.0f, 1.0f, 0.0f, 1.0f}); // зеленая
    vertices.push_back({v2_x, v2_y, v2_z, 0.0f, 0.0f, 1.0f, 1.0f}); // синяя

    // Грань 2: белая, синяя, красная
    vertices.push_back({top_x, top_y, top_z, 1.0f, 1.0f, 1.0f, 1.0f}); // белая
    vertices.push_back({v2_x, v2_y, v2_z, 0.0f, 0.0f, 1.0f, 1.0f}); // синяя
    vertices.push_back({v3_x, v3_y, v3_z, 1.0f, 0.0f, 0.0f, 1.0f}); // красная

    // Грань 3: белая, красная, зеленая
    vertices.push_back({top_x, top_y, top_z, 1.0f, 1.0f, 1.0f, 1.0f}); // белая
    vertices.push_back({v3_x, v3_y, v3_z, 1.0f, 0.0f, 0.0f, 1.0f}); // красная
    vertices.push_back({v1_x, v1_y, v1_z, 0.0f, 1.0f, 0.0f, 1.0f}); // зеленая

    // Грань 4 (основание): зеленая, красная, синяя
    vertices.push_back({v1_x, v1_y, v1_z, 0.0f, 1.0f, 0.0f, 1.0f}); // зеленая
    vertices.push_back({v3_x, v3_y, v3_z, 1.0f, 0.0f, 0.0f, 1.0f}); // красная
    vertices.push_back({v2_x, v2_y, v2_z, 0.0f, 0.0f, 1.0f, 1.0f}); // синяя

    return vertices;
}

// Инициализация VBO для тетраэдра
void Tetrahedron_InitVBO() {
    auto vertices = createTetrahedron();
    glGenBuffers(1, &VBO_Tetrahedron);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Tetrahedron);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex3DWithColor), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkOpenGLerror();
}

// Обработка клавиатуры для перемещения тетраэдра
void Tetrahedron_HandleKeyboard() {
    float speed = 0.02f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) tetrahedronPos.x -= speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) tetrahedronPos.x += speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) tetrahedronPos.y += speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) tetrahedronPos.y -= speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) tetrahedronPos.z -= speed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) tetrahedronPos.z += speed;
}

// Отрисовка тетраэдра
void Tetrahedron_Draw() {
    Tetrahedron_HandleKeyboard();
    
    glUseProgram(ProgramTetrahedron);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Tetrahedron);
    
    // Устанавливаем атрибуты вершин
    glEnableVertexAttribArray(Attrib_vertex_tetra);
    glVertexAttribPointer(Attrib_vertex_tetra, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3DWithColor), (void*)0);
    glEnableVertexAttribArray(Attrib_color_tetra);
    glVertexAttribPointer(Attrib_color_tetra, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3DWithColor), (void*)offsetof(Vertex3DWithColor, r));
    
    // Матрицы преобразования
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Передаем uniform-переменные в шейдер
    glUniformMatrix4fv(Uniform_projection_tetra, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(Uniform_view_tetra, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(Uniform_model_tetra, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(Uniform_offset_tetra, 1, glm::value_ptr(tetrahedronPos));
    
    // Отрисовываем тетраэдр
    glDrawArrays(GL_TRIANGLES, 0, 12);
    
    // Отключаем атрибуты
    glDisableVertexAttribArray(Attrib_vertex_tetra);
    glDisableVertexAttribArray(Attrib_color_tetra);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

// Освобождение ресурсов тетраэдра
void Tetrahedron_Release() {
    glDeleteBuffers(1, &VBO_Tetrahedron);
    glDeleteProgram(ProgramTetrahedron);
}

// ======================= ЗАДАНИЕ 2: КУБИК С ТЕКСТУРОЙ И ЦВЕТОМ =======================

GLuint VBO_Cube1, VAO_Cube1, ProgramCube1, textureID1;
float colorIntensity = 1.0f;
glm::vec3 baseColor(1.0f);
GLuint Attrib_vertex_cube1, Attrib_texcoord_cube1, Attrib_color_cube1;
GLuint Uniform_model_cube1, Uniform_view_cube1, Uniform_projection_cube1;
GLuint Uniform_colorIntensity_cube1, Uniform_baseColor_cube1;

// Шейдеры для первого куба с улучшенным смешиванием цветов (как у ребят)
const char* VertexShaderCube1 = R"(
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec4 vertexColor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec2 TexCoord;
out vec4 Color;
void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    TexCoord = texCoord;
    Color = vertexColor;
}
)";

const char* FragShaderCube1 = R"(
#version 330 core
in vec2 TexCoord;
in vec4 Color;
out vec4 FragColor;
uniform sampler2D textureSampler;
uniform float colorIntensity;
uniform vec3 baseColor;
void main() {
    vec4 textureColor = texture(textureSampler, TexCoord);
    
    // Улучшенное смешивание как у ребят
    vec4 coloredTexture = vec4(textureColor.rgb * baseColor, textureColor.a);
    
    // Смешиваем текстуру и окрашенную текстуру
    FragColor = mix(textureColor, coloredTexture, colorIntensity);
}
)";

// Структура вершины для куба с текстурой и цветом
struct CubeVertexWithColor {
    GLfloat x, y, z;
    GLfloat u, v;
    GLfloat r, g, b, a;
};

// Создание вершин куба с текстурными координатами и цветами (упрощенное как у ребят)
std::vector<CubeVertexWithColor> createCubeVerticesWithColor() {
    std::vector<CubeVertexWithColor> vertices;
    
    // Более простые цвета для граней как у ребят
    glm::vec4 frontColor(1.0f, 0.0f, 0.0f, 1.0f);    // красный
    glm::vec4 backColor(0.0f, 1.0f, 0.0f, 1.0f);     // зеленый
    glm::vec4 topColor(0.0f, 0.0f, 1.0f, 1.0f);      // синий
    glm::vec4 bottomColor(1.0f, 1.0f, 0.0f, 1.0f);   // желтый
    glm::vec4 rightColor(1.0f, 0.0f, 1.0f, 1.0f);    // фиолетовый
    glm::vec4 leftColor(0.0f, 1.0f, 1.0f, 1.0f);     // бирюзовый
    
    // Передняя грань (красная)
    vertices.push_back({-0.5f, -0.5f,  0.5f, 0.0f, 0.0f, frontColor.r, frontColor.g, frontColor.b, frontColor.a});
    vertices.push_back({0.5f, -0.5f,  0.5f, 1.0f, 0.0f, frontColor.r, frontColor.g, frontColor.b, frontColor.a});
    vertices.push_back({0.5f,  0.5f,  0.5f, 1.0f, 1.0f, frontColor.r, frontColor.g, frontColor.b, frontColor.a});
    vertices.push_back({-0.5f,  0.5f,  0.5f, 0.0f, 1.0f, frontColor.r, frontColor.g, frontColor.b, frontColor.a});
    
    // Задняя грань (зеленая)
    vertices.push_back({-0.5f, -0.5f, -0.5f, 1.0f, 0.0f, backColor.r, backColor.g, backColor.b, backColor.a});
    vertices.push_back({-0.5f,  0.5f, -0.5f, 1.0f, 1.0f, backColor.r, backColor.g, backColor.b, backColor.a});
    vertices.push_back({0.5f,  0.5f, -0.5f, 0.0f, 1.0f, backColor.r, backColor.g, backColor.b, backColor.a});
    vertices.push_back({0.5f, -0.5f, -0.5f, 0.0f, 0.0f, backColor.r, backColor.g, backColor.b, backColor.a});
    
    // Верхняя грань (синяя)
    vertices.push_back({-0.5f,  0.5f, -0.5f, 0.0f, 1.0f, topColor.r, topColor.g, topColor.b, topColor.a});
    vertices.push_back({-0.5f,  0.5f,  0.5f, 0.0f, 0.0f, topColor.r, topColor.g, topColor.b, topColor.a});
    vertices.push_back({0.5f,  0.5f,  0.5f, 1.0f, 0.0f, topColor.r, topColor.g, topColor.b, topColor.a});
    vertices.push_back({0.5f,  0.5f, -0.5f, 1.0f, 1.0f, topColor.r, topColor.g, topColor.b, topColor.a});
    
    // Нижняя грань (желтая)
    vertices.push_back({-0.5f, -0.5f, -0.5f, 1.0f, 1.0f, bottomColor.r, bottomColor.g, bottomColor.b, bottomColor.a});
    vertices.push_back({0.5f, -0.5f, -0.5f, 0.0f, 1.0f, bottomColor.r, bottomColor.g, bottomColor.b, bottomColor.a});
    vertices.push_back({0.5f, -0.5f,  0.5f, 0.0f, 0.0f, bottomColor.r, bottomColor.g, bottomColor.b, bottomColor.a});
    vertices.push_back({-0.5f, -0.5f,  0.5f, 1.0f, 0.0f, bottomColor.r, bottomColor.g, bottomColor.b, bottomColor.a});
    
    // Правая грань (фиолетовая)
    vertices.push_back({0.5f, -0.5f, -0.5f, 1.0f, 0.0f, rightColor.r, rightColor.g, rightColor.b, rightColor.a});
    vertices.push_back({0.5f,  0.5f, -0.5f, 1.0f, 1.0f, rightColor.r, rightColor.g, rightColor.b, rightColor.a});
    vertices.push_back({0.5f,  0.5f,  0.5f, 0.0f, 1.0f, rightColor.r, rightColor.g, rightColor.b, rightColor.a});
    vertices.push_back({0.5f, -0.5f,  0.5f, 0.0f, 0.0f, rightColor.r, rightColor.g, rightColor.b, rightColor.a});
    
    // Левая грань (бирюзовая)
    vertices.push_back({-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, leftColor.r, leftColor.g, leftColor.b, leftColor.a});
    vertices.push_back({-0.5f, -0.5f,  0.5f, 1.0f, 0.0f, leftColor.r, leftColor.g, leftColor.b, leftColor.a});
    vertices.push_back({-0.5f,  0.5f,  0.5f, 1.0f, 1.0f, leftColor.r, leftColor.g, leftColor.b, leftColor.a});
    vertices.push_back({-0.5f,  0.5f, -0.5f, 0.0f, 1.0f, leftColor.r, leftColor.g, leftColor.b, leftColor.a});
    
    return vertices;
}

// Индексы для куба
std::vector<GLuint> createCubeIndices() {
    std::vector<GLuint> indices;
    for (int i = 0; i < 6; i++) {
        GLuint base = i * 4;
        indices.push_back(base); indices.push_back(base+1); indices.push_back(base+2);
        indices.push_back(base); indices.push_back(base+2); indices.push_back(base+3);
    }
    return indices;
}

// Загрузка текстуры из файла
GLuint loadTexture(const std::string& filename) {
    sf::Image image;
    if (!image.loadFromFile(filename)) {
        std::cerr << "Не удалось загрузить текстуру: " << filename << std::endl;
        // Создаем текстуру-заглушку с плавным градиентом
        image.create(256, 256, sf::Color::Cyan);
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 256; x++) {
                float r = x / 255.0f;
                float g = y / 255.0f;
                float b = 0.5f + 0.5f * sin(x * 0.05f) * cos(y * 0.05f);
                image.setPixel(x, y, sf::Color(r*255, g*255, b*255, 255));
            }
        }
    }
    image.flipVertically();
    
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getSize().x, image.getSize().y, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

// Обработка клавиатуры для первого куба (как у ребят)
void Cube1_HandleKeyboard() {
    // Изменение интенсивности цвета с плавным шагом
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        colorIntensity -= 0.01f;
        if (colorIntensity < 0.0f) colorIntensity = 0.0f;
        std::cout << "Интенсивность цвета: " << colorIntensity << " (0=только текстура, 1=текстура+цвет)" << std::endl;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        colorIntensity += 0.01f;
        if (colorIntensity > 1.0f) colorIntensity = 1.0f;
        std::cout << "Интенсивность цвета: " << colorIntensity << " (0=только текстура, 1=текстура+цвет)" << std::endl;
    }
    
    // Изменение цветовых каналов с плавным шагом
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
        baseColor.r += 0.02f;
        if (baseColor.r > 2.0f) baseColor.r = 2.0f;
        std::cout << "Базовый цвет: R=" << baseColor.r << " G=" << baseColor.g << " B=" << baseColor.b << std::endl;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        baseColor.r -= 0.02f;
        if (baseColor.r < 0.0f) baseColor.r = 0.0f;
        std::cout << "Базовый цвет: R=" << baseColor.r << " G=" << baseColor.g << " B=" << baseColor.b << std::endl;
    }
    
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        baseColor.g += 0.02f;
        if (baseColor.g > 2.0f) baseColor.g = 2.0f;
        std::cout << "Базовый цвет: R=" << baseColor.r << " G=" << baseColor.g << " B=" << baseColor.b << std::endl;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        baseColor.g -= 0.02f;
        if (baseColor.g < 0.0f) baseColor.g = 0.0f;
        std::cout << "Базовый цвет: R=" << baseColor.r << " G=" << baseColor.g << " B=" << baseColor.b << std::endl;
    }
    
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)) {
        baseColor.b += 0.02f;
        if (baseColor.b > 2.0f) baseColor.b = 2.0f;
        std::cout << "Базовый цвет: R=" << baseColor.r << " G=" << baseColor.g << " B=" << baseColor.b << std::endl;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        baseColor.b -= 0.02f;
        if (baseColor.b < 0.0f) baseColor.b = 0.0f;
        std::cout << "Базовый цвет: R=" << baseColor.r << " G=" << baseColor.g << " B=" << baseColor.b << std::endl;
    }
    
    // Сброс настроек
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
        colorIntensity = 1.0f;
        baseColor = glm::vec3(1.0f);
        std::cout << "Настройки сброшены: белый цвет, полная интенсивность" << std::endl;
    }
    
    // Только текстура
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::T)) {
        colorIntensity = 0.0f;
        std::cout << "Режим: только текстура" << std::endl;
    }
}

// Инициализация шейдеров для первого куба
void Cube1_InitShader() {
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &VertexShaderCube1, NULL);
    glCompileShader(vShader);
    ShaderLog(vShader);
    
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &FragShaderCube1, NULL);
    glCompileShader(fShader);
    ShaderLog(fShader);
    
    ProgramCube1 = glCreateProgram();
    glAttachShader(ProgramCube1, vShader);
    glAttachShader(ProgramCube1, fShader);
    glLinkProgram(ProgramCube1);
    
    // Получаем местоположения атрибутов
    Attrib_vertex_cube1 = glGetAttribLocation(ProgramCube1, "position");
    Attrib_texcoord_cube1 = glGetAttribLocation(ProgramCube1, "texCoord");
    Attrib_color_cube1 = glGetAttribLocation(ProgramCube1, "vertexColor");
    
    // Получаем местоположения uniform-переменных
    Uniform_model_cube1 = glGetUniformLocation(ProgramCube1, "model");
    Uniform_view_cube1 = glGetUniformLocation(ProgramCube1, "view");
    Uniform_projection_cube1 = glGetUniformLocation(ProgramCube1, "projection");
    Uniform_colorIntensity_cube1 = glGetUniformLocation(ProgramCube1, "colorIntensity");
    Uniform_baseColor_cube1 = glGetUniformLocation(ProgramCube1, "baseColor");
    
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    checkOpenGLerror();
}

// Инициализация VBO и VAO для первого куба
void Cube1_InitVBO() {
    auto vertices = createCubeVerticesWithColor();
    auto indices = createCubeIndices();
    
    glGenVertexArrays(1, &VAO_Cube1);
    glBindVertexArray(VAO_Cube1);
    
    glGenBuffers(1, &VBO_Cube1);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Cube1);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(CubeVertexWithColor), vertices.data(), GL_STATIC_DRAW);
    
    GLuint IBO;
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    
    // Настраиваем атрибуты вершин
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertexWithColor), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(CubeVertexWithColor), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(CubeVertexWithColor), (void*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    // Загружаем текстуру
    textureID1 = loadTexture("assets/textures/0.jpg");
    checkOpenGLerror();
}

// Отрисовка первого куба с измененной траекторией вращения
void Cube1_Draw() {
    Cube1_HandleKeyboard();
    
    glUseProgram(ProgramCube1);
    glBindVertexArray(VAO_Cube1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID1);
    
    // Автоматическое вращение куба с измененной траекторией
    static float rotationAngle = 0.0f;
    rotationAngle += 0.5f;
    
    // Матрицы преобразования с измененным вращением
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Новая траектория вращения: вращение по всем трем осям с разными скоростями
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)); // Y-ось
    model = glm::rotate(model, glm::radians(rotationAngle * 0.7f), glm::vec3(1.0f, 0.0f, 0.0f)); // X-ось
    model = glm::rotate(model, glm::radians(rotationAngle * 0.3f), glm::vec3(0.0f, 0.0f, 1.0f)); // Z-ось
    
    // Передаем uniform-переменные
    glUniformMatrix4fv(Uniform_model_cube1, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(Uniform_view_cube1, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(Uniform_projection_cube1, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(Uniform_colorIntensity_cube1, colorIntensity);
    glUniform3fv(Uniform_baseColor_cube1, 1, glm::value_ptr(baseColor));
    
    // Отрисовываем куб
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0);
    glUseProgram(0);
}

// Освобождение ресурсов первого куба
void Cube1_Release() {
    glDeleteBuffers(1, &VBO_Cube1);
    glDeleteVertexArrays(1, &VAO_Cube1);
    glDeleteProgram(ProgramCube1);
    glDeleteTextures(1, &textureID1);
}

// ======================= ЗАДАНИЕ 3: КУБИК С ДВУМЯ ТЕКСТУРАМИ =======================

GLuint VBO_Cube2, VAO_Cube2, ProgramCube2, textureID2_1, textureID2_2;
float textureMix = 0.5f;
GLuint Attrib_vertex_cube2, Attrib_texcoord_cube2;
GLuint Uniform_model_cube2, Uniform_view_cube2, Uniform_projection_cube2, Uniform_textureMix_cube2;

// Шейдеры для второго куба
const char* VertexShaderCube2 = R"(
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec2 TexCoord;
void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    TexCoord = texCoord;
}
)";

const char* FragShaderCube2 = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D textureSampler1;
uniform sampler2D textureSampler2;
uniform float textureMix;
void main() {
    vec4 tex1 = texture(textureSampler1, TexCoord);
    vec4 tex2 = texture(textureSampler2, TexCoord);
    FragColor = mix(tex1, tex2, textureMix);
}
)";

// Вершины для куба (без цвета)
std::vector<Vertex3DWithTex> createCubeVertices() {
    std::vector<Vertex3DWithTex> vertices = {
        // Передняя грань
        {-0.5f, -0.5f,  0.5f, 0.0f, 0.0f}, {0.5f, -0.5f,  0.5f, 1.0f, 0.0f},
        {0.5f,  0.5f,  0.5f, 1.0f, 1.0f}, {-0.5f,  0.5f,  0.5f, 0.0f, 1.0f},
        // Задняя грань
        {-0.5f, -0.5f, -0.5f, 1.0f, 0.0f}, {-0.5f,  0.5f, -0.5f, 1.0f, 1.0f},
        {0.5f,  0.5f, -0.5f, 0.0f, 1.0f}, {0.5f, -0.5f, -0.5f, 0.0f, 0.0f},
        // Верхняя грань
        {-0.5f,  0.5f, -0.5f, 0.0f, 1.0f}, {-0.5f,  0.5f,  0.5f, 0.0f, 0.0f},
        {0.5f,  0.5f,  0.5f, 1.0f, 0.0f}, {0.5f,  0.5f, -0.5f, 1.0f, 1.0f},
        // Нижняя грань
        {-0.5f, -0.5f, -0.5f, 1.0f, 1.0f}, {0.5f, -0.5f, -0.5f, 0.0f, 1.0f},
        {0.5f, -0.5f,  0.5f, 0.0f, 0.0f}, {-0.5f, -0.5f,  0.5f, 1.0f, 0.0f},
        // Правая грань
        {0.5f, -0.5f, -0.5f, 1.0f, 0.0f}, {0.5f,  0.5f, -0.5f, 1.0f, 1.0f},
        {0.5f,  0.5f,  0.5f, 0.0f, 1.0f}, {0.5f, -0.5f,  0.5f, 0.0f, 0.0f},
        // Левая грань
        {-0.5f, -0.5f, -0.5f, 0.0f, 0.0f}, {-0.5f, -0.5f,  0.5f, 1.0f, 0.0f},
        {-0.5f,  0.5f,  0.5f, 1.0f, 1.0f}, {-0.5f,  0.5f, -0.5f, 0.0f, 1.0f}
    };
    return vertices;
}

// Обработка клавиатуры для второго куба
void Cube2_HandleKeyboard() {
    // Управление пропорцией смешивания текстур
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        textureMix -= 0.005f;
        if (textureMix < 0.0f) textureMix = 0.0f;
        std::cout << "Смешивание текстур: " << textureMix << " (0=только первая, 1=только вторая)" << std::endl;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        textureMix += 0.005f;
        if (textureMix > 1.0f) textureMix = 1.0f;
        std::cout << "Смешивание текстур: " << textureMix << " (0=только первая, 1=только вторая)" << std::endl;
    }
}

// Инициализация шейдеров для второго куба
void Cube2_InitShader() {
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &VertexShaderCube2, NULL);
    glCompileShader(vShader);
    ShaderLog(vShader);
    
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &FragShaderCube2, NULL);
    glCompileShader(fShader);
    ShaderLog(fShader);
    
    ProgramCube2 = glCreateProgram();
    glAttachShader(ProgramCube2, vShader);
    glAttachShader(ProgramCube2, fShader);
    glLinkProgram(ProgramCube2);
    
    Attrib_vertex_cube2 = glGetAttribLocation(ProgramCube2, "position");
    Attrib_texcoord_cube2 = glGetAttribLocation(ProgramCube2, "texCoord");
    Uniform_model_cube2 = glGetUniformLocation(ProgramCube2, "model");
    Uniform_view_cube2 = glGetUniformLocation(ProgramCube2, "view");
    Uniform_projection_cube2 = glGetUniformLocation(ProgramCube2, "projection");
    Uniform_textureMix_cube2 = glGetUniformLocation(ProgramCube2, "textureMix");
    
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    checkOpenGLerror();
}

// Инициализация VBO и VAO для второго куба
void Cube2_InitVBO() {
    auto vertices = createCubeVertices();
    auto indices = createCubeIndices();
    
    glGenVertexArrays(1, &VAO_Cube2);
    glBindVertexArray(VAO_Cube2);
    
    glGenBuffers(1, &VBO_Cube2);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Cube2);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex3DWithTex), vertices.data(), GL_STATIC_DRAW);
    
    GLuint IBO;
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    
    // Настраиваем атрибуты вершин
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3DWithTex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3DWithTex), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    // Загружаем две текстуры
    textureID2_1 = loadTexture("assets/textures/1.jpg");
    textureID2_2 = loadTexture("assets/textures/2.jpg");
    checkOpenGLerror();
}

// Отрисовка второго куба с измененной траекторией вращения
void Cube2_Draw() {
    Cube2_HandleKeyboard();
    
    glUseProgram(ProgramCube2);
    glBindVertexArray(VAO_Cube2);
    
    // Активируем и привязываем первую текстуру
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID2_1);
    
    // Активируем и привязываем вторую текстуру
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureID2_2);
    
    // Устанавливаем uniform-переменные для сэмплеров
    glUniform1i(glGetUniformLocation(ProgramCube2, "textureSampler1"), 0);
    glUniform1i(glGetUniformLocation(ProgramCube2, "textureSampler2"), 1);
    
    // Автоматическое вращение куба с новой траекторией
    static float rotationAngle = 0.0f;
    rotationAngle += 0.5f;
    
    // Матрицы преобразования с измененным вращением
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Новая траектория вращения: вращение по всем трем осям с разными скоростями
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotationAngle * 0.5f), glm::vec3(0.0f, 1.0f, 0.0f)); // Y-ось (медленнее)
    model = glm::rotate(model, glm::radians(rotationAngle * 1.2f), glm::vec3(1.0f, 0.0f, 0.0f)); // X-ось (быстрее)
    model = glm::rotate(model, glm::radians(rotationAngle * 0.9f), glm::vec3(0.0f, 0.0f, 1.0f)); // Z-ось
    
    // Передаем uniform-переменные
    glUniformMatrix4fv(Uniform_model_cube2, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(Uniform_view_cube2, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(Uniform_projection_cube2, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(Uniform_textureMix_cube2, textureMix);
    
    // Отрисовываем куб
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0);
    glUseProgram(0);
}

// Освобождение ресурсов второго куба
void Cube2_Release() {
    glDeleteBuffers(1, &VBO_Cube2);
    glDeleteVertexArrays(1, &VAO_Cube2);
    glDeleteProgram(ProgramCube2);
    glDeleteTextures(1, &textureID2_1);
    glDeleteTextures(1, &textureID2_2);
}

// ======================= ЗАДАНИЕ 4: ГРАДИЕНТНЫЙ КРУГ =======================

GLuint VBO_Circle, ProgramCircle;
glm::vec2 circleScale(1.0f);
GLuint Attrib_vertex_circle, Attrib_color_circle;
GLuint Uniform_scale_circle;

// Шейдеры для круга
const char* VertexShaderCircle = R"(
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
uniform vec2 scale;
out vec4 vertexColor;
void main() {
    vec3 scaledPosition = position;
    scaledPosition.x *= scale.x;
    scaledPosition.y *= scale.y;
    gl_Position = vec4(scaledPosition, 1.0);
    vertexColor = color;
}
)";

const char* FragShaderCircle = R"(
#version 330 core
in vec4 vertexColor;
out vec4 FragColor;
void main() {
    FragColor = vertexColor;
}
)";

// Улучшенное преобразование HSV в RGB с плавными переходами
glm::vec3 HSVtoRGB(float h, float s, float v) {
    float hh = fmod(h, 360.0f) / 60.0f;
    float c = v * s;
    float x = c * (1.0f - fabs(fmod(hh, 2.0f) - 1.0f));
    float m = v - c;
    
    glm::vec3 rgb(0.0f);
    
    if (hh >= 0 && hh < 1) rgb = glm::vec3(c, x, 0);
    else if (hh >= 1 && hh < 2) rgb = glm::vec3(x, c, 0);
    else if (hh >= 2 && hh < 3) rgb = glm::vec3(0, c, x);
    else if (hh >= 3 && hh < 4) rgb = glm::vec3(0, x, c);
    else if (hh >= 4 && hh < 5) rgb = glm::vec3(x, 0, c);
    else rgb = glm::vec3(c, 0, x);
    
    rgb += m;
    
    // Гарантируем, что значения находятся в диапазоне [0, 1]
    rgb.r = glm::clamp(rgb.r, 0.0f, 1.0f);
    rgb.g = glm::clamp(rgb.g, 0.0f, 1.0f);
    rgb.b = glm::clamp(rgb.b, 0.0f, 1.0f);
    
    return rgb;
}

// Создание вершин градиентного круга с плавными переходами
std::vector<Vertex3DWithColor> createGradientCircle() {
    std::vector<Vertex3DWithColor> vertices;
    int segments = 360; // Увеличиваем количество сегментов для плавности
    float radius = 0.5f;
    
    // Центр круга (белый)
    vertices.push_back({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f});
    
    // Точки по окружности с плавным градиентом по Hue
    for (int i = 0; i <= segments; i++) {
        float angle = (i / (float)segments) * 2.0f * 3.14159265f;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        
        // Плавное изменение Hue по окружности
        float hue = (i / (float)segments) * 360.0f;
        
        // Добавляем небольшую вариацию для более плавных переходов
        float hueVariation = sin(angle * 2.0f) * 5.0f; // Небольшие колебания для плавности
        hue += hueVariation;
        
        glm::vec3 color = HSVtoRGB(hue, 1.0f, 1.0f);
        
        vertices.push_back({x, y, 0.0f, color.r, color.g, color.b, 1.0f});
    }
    
    return vertices;
}

// Обработка клавиатуры для круга
void Circle_HandleKeyboard() {
    float speed = 0.02f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        circleScale.x -= speed;
        if (circleScale.x < 0.1f) circleScale.x = 0.1f;
        std::cout << "Масштаб по X: " << circleScale.x << std::endl;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        circleScale.x += speed;
        if (circleScale.x > 3.0f) circleScale.x = 3.0f;
        std::cout << "Масштаб по X: " << circleScale.x << std::endl;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        circleScale.y -= speed;
        if (circleScale.y < 0.1f) circleScale.y = 0.1f;
        std::cout << "Масштаб по Y: " << circleScale.y << std::endl;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        circleScale.y += speed;
        if (circleScale.y > 3.0f) circleScale.y = 3.0f;
        std::cout << "Масштаб по Y: " << circleScale.y << std::endl;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
        circleScale = glm::vec2(1.0f);
        std::cout << "Масштаб сброшен к 1.0" << std::endl;
    }
}

// Инициализация шейдеров для круга
void Circle_InitShader() {
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &VertexShaderCircle, NULL);
    glCompileShader(vShader);
    ShaderLog(vShader);
    
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &FragShaderCircle, NULL);
    glCompileShader(fShader);
    ShaderLog(fShader);
    
    ProgramCircle = glCreateProgram();
    glAttachShader(ProgramCircle, vShader);
    glAttachShader(ProgramCircle, fShader);
    glLinkProgram(ProgramCircle);
    
    Attrib_vertex_circle = glGetAttribLocation(ProgramCircle, "position");
    Attrib_color_circle = glGetAttribLocation(ProgramCircle, "color");
    Uniform_scale_circle = glGetUniformLocation(ProgramCircle, "scale");
    
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    checkOpenGLerror();
}

// Инициализация VBO для круга
void Circle_InitVBO() {
    auto vertices = createGradientCircle();
    glGenBuffers(1, &VBO_Circle);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Circle);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex3DWithColor), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkOpenGLerror();
}

// Отрисовка круга
void Circle_Draw() {
    Circle_HandleKeyboard();
    
    glUseProgram(ProgramCircle);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Circle);
    
    // Устанавливаем атрибуты вершин
    glEnableVertexAttribArray(Attrib_vertex_circle);
    glVertexAttribPointer(Attrib_vertex_circle, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3DWithColor), (void*)0);
    glEnableVertexAttribArray(Attrib_color_circle);
    glVertexAttribPointer(Attrib_color_circle, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3DWithColor), (void*)offsetof(Vertex3DWithColor, r));
    
    // Передаем uniform-переменную масштаба
    glUniform2fv(Uniform_scale_circle, 1, glm::value_ptr(circleScale));
    
    // Отрисовываем круг как треугольный веер
    glDrawArrays(GL_TRIANGLE_FAN, 0, 362); // 1 центр + 361 точка по окружности
    
    // Отключаем атрибуты
    glDisableVertexAttribArray(Attrib_vertex_circle);
    glDisableVertexAttribArray(Attrib_color_circle);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

// Освобождение ресурсов круга
void Circle_Release() {
    glDeleteBuffers(1, &VBO_Circle);
    glDeleteProgram(ProgramCircle);
}

// ======================= ГЛАВНЫЕ ФУНКЦИИ УПРАВЛЕНИЯ =======================

// Инициализация текущего задания
void Init() {
    switch (currentAssignment) {
        case ASSIGNMENT_1: 
            Tetrahedron_InitShader(); 
            Tetrahedron_InitVBO(); 
            break;
        case ASSIGNMENT_2: 
            Cube1_InitShader(); 
            Cube1_InitVBO(); 
            break;
        case ASSIGNMENT_3: 
            Cube2_InitShader(); 
            Cube2_InitVBO(); 
            break;
        case ASSIGNMENT_4: 
            Circle_InitShader(); 
            Circle_InitVBO(); 
            break;
    }
    glEnable(GL_DEPTH_TEST);
}

// Отрисовка текущего задания
void Draw() {
    switch (currentAssignment) {
        case ASSIGNMENT_1: Tetrahedron_Draw(); break;
        case ASSIGNMENT_2: Cube1_Draw(); break;
        case ASSIGNMENT_3: Cube2_Draw(); break;
        case ASSIGNMENT_4: Circle_Draw(); break;
    }
}

// Освобождение ресурсов текущего задания
void Release() {
    switch (currentAssignment) {
        case ASSIGNMENT_1: Tetrahedron_Release(); break;
        case ASSIGNMENT_2: Cube1_Release(); break;
        case ASSIGNMENT_3: Cube2_Release(); break;
        case ASSIGNMENT_4: Circle_Release(); break;
    }
}

// ======================= ГЛАВНАЯ ФУНКЦИЯ =======================

int main() {
    setlocale(LC_ALL, "ru");
    
    // Создаем окно SFML
    sf::Window window(sf::VideoMode(900, 900), "Lab 12", 
                     sf::Style::Default, sf::ContextSettings(32));
    window.setVerticalSyncEnabled(true);
    window.setActive(true);
    
    // Инициализируем GLEW
    GLenum glewResult = glewInit();
    if (glewResult != GLEW_OK) {
        std::cerr << "Ошибка GLEW: " << glewGetErrorString(glewResult) << std::endl;
        return -1;
    }
    
    std::cout << "Клавиши переключения заданий: 1, 2, 3, 4" << std::endl;
    std::cout << std::endl;
    std::cout << "Текущее задание: Тетраэдр (#1)" << std::endl;
    std::cout << "Управление: стрелки для перемещения, W/S для оси Z" << std::endl;
    
    // Инициализация
    Init();
    
    // Главный цикл
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::Resized) {
                glViewport(0, 0, event.size.width, event.size.height);
            }
            else if (event.type == sf::Event::KeyPressed) {
                Assignment prev = currentAssignment;
                
                // Переключение заданий
                if (event.key.code == sf::Keyboard::Num1) {
                    currentAssignment = ASSIGNMENT_1;
                    std::cout << "Активирован тетраэдр" << std::endl;
                }
                else if (event.key.code == sf::Keyboard::Num2) {
                    currentAssignment = ASSIGNMENT_2;
                    std::cout << "Активирован кубик с цветом" << std::endl;
                }
                else if (event.key.code == sf::Keyboard::Num3) {
                    currentAssignment = ASSIGNMENT_3;
                    std::cout << "Активирован кубик с двумя текстурами" << std::endl;
                }
                else if (event.key.code == sf::Keyboard::Num4) {
                    currentAssignment = ASSIGNMENT_4;
                    std::cout << "Активирован градиентный круг" << std::endl;
                }
                
                // Переинициализация при смене задания
                if (prev != currentAssignment) {
                    Release();
                    Init();
                }
            }
        }
        
        // Очистка буферов
        glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Отрисовка
        Draw();
        
        // Отображение
        window.display();
    }
    
    // Освобождение ресурсов
    Release();
    return 0;
}