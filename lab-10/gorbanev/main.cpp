#include <SFML/Window.hpp>
#include <GL/glew.h>
#include <iostream>
#include <vector>
#include <string>

struct Vertex {
    GLfloat x;
    GLfloat y;
};

GLuint Program;
GLint Attrib_vertex;
GLuint VBO;

const char* VertexShaderSource = R"(
#version 330 core
in vec2 coord;
void main() {
    gl_Position = vec4(coord, 0.0, 1.0);
}
)";

const char* FragShaderSource = R"(
#version 330 core
out vec4 color;
void main() {
    color = vec4(0, 1, 0, 1);
}
)";

void checkOpenGLerror() {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error! Code: " << err << std::endl;
    }
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
    // Вершинный шейдер
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &VertexShaderSource, NULL);
    glCompileShader(vShader);
    std::cout << "vertex shader \n";
    ShaderLog(vShader);
    checkOpenGLerror();

    // Фрагментный шейдер
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &FragShaderSource, NULL);
    glCompileShader(fShader);
    std::cout << "fragment shader \n";
    ShaderLog(fShader);
    checkOpenGLerror();

    // Шейдерная программа
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

    // Получаем ID атрибута
    const char* attr_name = "coord";
    Attrib_vertex = glGetAttribLocation(Program, attr_name);
    if (Attrib_vertex == -1) {
        std::cout << "could not bind attrib " << attr_name << std::endl;
        return;
    }

    // Удаляем шейдеры, так как они уже включены в программу
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    
    checkOpenGLerror();
}

void InitVBO() {
    glGenBuffers(1, &VBO);
    
    // Вершины треугольника
    Vertex triangle[3] = {
        { -1.0f, -1.0f },  // Левый нижний
        {  0.0f,  1.0f },  // Верх
        {  1.0f, -1.0f }   // Правый нижний
    };

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
    
    checkOpenGLerror();
}

void Init() {
    InitShader();
    InitVBO();
}

void Draw() {
    glUseProgram(Program);
    glEnableVertexAttribArray(Attrib_vertex);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(Attrib_vertex, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // Отключаем VBO после настройки атрибутов
    
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    glDisableVertexAttribArray(Attrib_vertex);
    glUseProgram(0);
    
    checkOpenGLerror();
}

void ReleaseVBO() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &VBO);
}

void ReleaseShader() {
    glUseProgram(0);
    glDeleteProgram(Program);
}

void Release() {
    ReleaseShader();
    ReleaseVBO();
}

int main() {
    sf::Window window(sf::VideoMode(600, 600), "Green Triangle - OpenGL", 
                     sf::Style::Default, sf::ContextSettings(24));
    window.setVerticalSyncEnabled(true);
    window.setActive(true);

    // Инициализация GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    Init();

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            else if (event.type == sf::Event::Resized) {
                glViewport(0, 0, event.size.width, event.size.height);
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        Draw();
        window.display();
    }

    Release();
    return 0;
}