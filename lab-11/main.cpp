#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <iostream>

enum Figure { DIAMOND, FAN, PENTAGON, FADE_DIAMOND, FADE_FAN, FADE_PENTAGON };

// ID шейдерной программы
GLuint Program;
// ID вершинного атрибута
GLint Attrib_vertex;
// ID Vertex Buffer Object
GLuint VBO;

GLint Unif_color;

GLint Attrib_color;

struct Vertex {
    GLfloat x;
    GLfloat y;
};

struct VertexColor {
    GLfloat x;
    GLfloat y;
    GLfloat r, g, b;
};

// Исходный код вершинного шейдера
const char* VertexShaderSource = R"(
#version 330 core
in vec2 coord;
void main() {
    gl_Position = vec4(coord, 0.0, 1.0);
}
)"; 

// Исходный код фрагментного шейдера
const char* FragShaderSource = R"(
#version 330 core
out vec4 color;
void main() {
    color = vec4(0.1, 0.5, 0.6, 1);
}
)";

const char* FragShaderUniformColorSource = R"(
#version 330 core
uniform vec4 u_color;
out vec4 color;
void main() {
    color = u_color;
}
)";

const char* VertexShaderFadeSource = R"(
#version 330 core
in vec2 coord;
in vec3 color;

out vec3 vert_color;

void main() {
    gl_Position = vec4(coord, 0.0, 1.0);
    vert_color = color;
}
)";

const char* FragShaderFadeSource = R"(
#version 330 core
in vec3 vert_color;
out vec4 color;

void main() {
    color = vec4(vert_color, 1.0); // Добавляем альфа-канал (1.0)
}
)";

void checkOpenGLerror() {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "error! code " << err << std::endl;
    }
}

void InitVBODiamond() {
    glGenBuffers(1, &VBO);
    Vertex square[4] = {
        { 0.0f, -0.9f },
        { 0.4f, 0.0f },
        { 0.0f, 0.9f },
        { -0.4f, 0.0f }
    };
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(square), square, GL_STATIC_DRAW);
    checkOpenGLerror();
}

void InitVBOFan() {
    glGenBuffers(1, &VBO);
    Vertex fan[6] = {
        { -0.1f, -0.4f },
        { -0.9f, -0.2f },
        { -0.65f, 0.2f },
        { -0.1f, 0.5f },
        { 0.45f, 0.5f },
        { 0.9f, -0.4f }
    };
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fan), fan, GL_STATIC_DRAW);
    checkOpenGLerror();
}

void InitVBOPentagon() {
    glGenBuffers(1, &VBO);
    Vertex pentagon[5] = {
        { -0.4f, -0.5f },
        { -0.75f, 0.2f },
        { 0.0f, 0.8f },
        { 0.75f, 0.2f },
        { 0.4f, -0.5f }
    };
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pentagon), pentagon, GL_STATIC_DRAW);
    checkOpenGLerror();
}

void InitVBODiamondFade() {
    glGenBuffers(1, &VBO);
    // x, y, r, g, b
    VertexColor square[4] = {
        { 0.0f, -0.9f,   1.0f, 0.0f, 0.0f },
        { 0.4f,  0.0f,   0.0f, 1.0f, 0.0f },
        { 0.0f,  0.9f,   0.0f, 0.0f, 1.0f },
        {-0.4f,  0.0f,   1.0f, 1.0f, 0.0f }
    };
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(square), square, GL_STATIC_DRAW);
    checkOpenGLerror();
}

void InitVBOFanFade() {
    glGenBuffers(1, &VBO);
    VertexColor fan[6] = {
        { -0.1f, -0.4f, 1.0f, 0.0f, 0.0f },
        { -0.9f, -0.2f, 0.0f, 1.0f, 0.0f},
        { -0.65f, 0.2f, 0.0f, 0.0f, 1.0f },
        { -0.1f, 0.5f,  1.0f, 1.0f, 0.0f },
        { 0.45f, 0.5f,  1.0f, 0.0f, 1.0f },
        { 0.9f, -0.4f,  0.0f, 1.0f, 1.0f }
    };
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fan), fan, GL_STATIC_DRAW);
    checkOpenGLerror();
}

void InitVBOPentagonFade() {
    glGenBuffers(1, &VBO);
    VertexColor pentagon[5] = {
        { -0.4f, -0.5f, 1.0f, 0.0f, 0.0f},
        { -0.75f, 0.2f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.8f,   0.0f, 0.0f, 1.0f },
        { 0.75f, 0.2f,  1.0f, 0.0f, 1.0f },
        { 0.4f, -0.5f,  0.0f, 1.0f, 1.0f }
    };
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pentagon), pentagon, GL_STATIC_DRAW);
    checkOpenGLerror();
}

void ShaderLog(unsigned int shader)
{
    int infologLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
    if (infologLen > 1)
    {
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
    checkOpenGLerror();

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &FragShaderSource, NULL);
    glCompileShader(fShader);
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
    checkOpenGLerror();
}

void InitUniformShader(float r, float g, float b) {
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &VertexShaderSource, NULL);
    glCompileShader(vShader);
    checkOpenGLerror();

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &FragShaderUniformColorSource, NULL);
    glCompileShader(fShader);
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

    const char* unif_name = "u_color";
    Unif_color = glGetUniformLocation(Program, unif_name);
    if (Unif_color == -1) {
        std::cerr << "unable to bind uniform! error" << std::endl;
        return;
    }

    glUseProgram(Program);
    glUniform4f(Unif_color, r, g, b, 1.0f);

    checkOpenGLerror();
}

void InitFadeShader() {
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &VertexShaderFadeSource, NULL);
    glCompileShader(vShader);
    checkOpenGLerror();

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &FragShaderFadeSource, NULL);
    glCompileShader(fShader);
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
    Attrib_color = glGetAttribLocation(Program, "color");
    if (Attrib_vertex == -1 || Attrib_color == -1) {
        std::cout << "could not bind attrib" << std::endl;
        return;
    }
    checkOpenGLerror();
}

void InitDiamond(bool use_uniform = false) {
    if (use_uniform) {
        InitUniformShader(1.0f, 0.0f, 0.0f);
    } else {
        InitShader();
    }
    InitVBODiamond();
}

void InitFan(bool use_uniform = false) {
    if (use_uniform) {
        InitUniformShader(0.0f, 1.0f, 0.0f);
    } else {
        InitShader();
    }
    InitVBOFan();
}

void InitPentagon(bool use_uniform = false) {
    if (use_uniform) {
        InitUniformShader(0.0f, 0.0f, 1.0f);
    } else {
        InitShader();
    }
    InitVBOPentagon();
}

void DrawDiamond() {
    glUseProgram(Program); // Устанавливаем шейдерную программу текущей
    glEnableVertexAttribArray(Attrib_vertex); // Включаем массив атрибутов
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // Подключаем VBO
    // сообщаем OpenGL как он должен интерпретировать вершинные данные.
    glVertexAttribPointer(Attrib_vertex, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Отключаем VBO
    glDrawArrays(GL_QUADS, 0, 4); // Рисуем
    glDisableVertexAttribArray(Attrib_vertex); // Отключаем массив атрибутов
    glUseProgram(0); // Отключаем шейдерную программу
    checkOpenGLerror();
}

void DrawFan() {
    glUseProgram(Program); // Устанавливаем шейдерную программу текущей
    glEnableVertexAttribArray(Attrib_vertex); // Включаем массив атрибутов
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // Подключаем VBO
    // сообщаем OpenGL как он должен интерпретировать вершинные данные.
    glVertexAttribPointer(Attrib_vertex, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Отключаем VBO
    glDrawArrays(GL_TRIANGLE_FAN, 0, 6); // Рисуем
    glDisableVertexAttribArray(Attrib_vertex); // Отключаем массив атрибутов
    glUseProgram(0); // Отключаем шейдерную программу
    checkOpenGLerror();
}

void DrawPentagon() {
    glUseProgram(Program); // Устанавливаем шейдерную программу текущей
    glEnableVertexAttribArray(Attrib_vertex); // Включаем массив атрибутов
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // Подключаем VBO
    // сообщаем OpenGL как он должен интерпретировать вершинные данные.
    glVertexAttribPointer(Attrib_vertex, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Отключаем VBO
    glDrawArrays(GL_TRIANGLE_FAN, 0, 5); // Рисуем
    glDisableVertexAttribArray(Attrib_vertex); // Отключаем массив атрибутов
    glUseProgram(0); // Отключаем шейдерную программу
    checkOpenGLerror();
}

void DrawFadeDiamond() {
    glUseProgram(Program);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glEnableVertexAttribArray(Attrib_vertex);
    glVertexAttribPointer(Attrib_vertex, 2, GL_FLOAT, GL_FALSE, sizeof(VertexColor), (void*)0);

    glEnableVertexAttribArray(Attrib_color);
    glVertexAttribPointer(Attrib_color, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor), (void*)(2 * sizeof(GLfloat)));

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(Attrib_vertex);
    glDisableVertexAttribArray(Attrib_color);
    glUseProgram(0);
}

void DrawFadeFan() {
    glUseProgram(Program);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glEnableVertexAttribArray(Attrib_vertex);
    glVertexAttribPointer(Attrib_vertex, 2, GL_FLOAT, GL_FALSE, sizeof(VertexColor), (void*)0);

    glEnableVertexAttribArray(Attrib_color);
    glVertexAttribPointer(Attrib_color, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor), (void*)(2 * sizeof(GLfloat)));

    glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

    glDisableVertexAttribArray(Attrib_vertex);
    glDisableVertexAttribArray(Attrib_color);
    glUseProgram(0);
}

void DrawFadePentagon() {
    glUseProgram(Program);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glEnableVertexAttribArray(Attrib_vertex);
    glVertexAttribPointer(Attrib_vertex, 2, GL_FLOAT, GL_FALSE, sizeof(VertexColor), (void*)0);

    glEnableVertexAttribArray(Attrib_color);
    glVertexAttribPointer(Attrib_color, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor), (void*)(2 * sizeof(GLfloat)));

    glDrawArrays(GL_TRIANGLE_FAN, 0, 5);

    glDisableVertexAttribArray(Attrib_vertex);
    glDisableVertexAttribArray(Attrib_color);
    glUseProgram(0);
}

// Освобождение буфера
void ReleaseVBO() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &VBO);
}

// Освобождение шейдеров
void ReleaseShader() {
    // Передавая ноль, мы отключаем шейдерную программу
    glUseProgram(0);
    // Удаляем шейдерную программу
    glDeleteProgram(Program);
}

void Release() {
    // Шейдеры
    ReleaseShader();
    // Вершинный буфер
    ReleaseVBO();
}

void print_options() {
    std::cout << "Клавиши" << std::endl;
    std::cout << "1. A - нарисовать четырехугольник" << std::endl;
    std::cout << "2. S - нарисовать веер" << std::endl;
    std::cout << "3. D - нарисовать пятиугольник" << std::endl;
    std::cout << "4. U - переключить режим передачи цвета (хардкод / uniform)" << std::endl;
    std::cout << "5. Q - нарисовать ромб с градиентом" << std::endl;
    std::cout << "6. W - нарисовать веер с градиентом" << std::endl;
    std::cout << "7. E - нарисовать пятиугольник с градиентом" << std::endl;
}

int main() {
    sf::Window window(sf::VideoMode(600, 600), "My OpenGL window", sf::Style::Default, sf::ContextSettings(24));
    window.setVerticalSyncEnabled(true);
    window.setActive(true);
    glewInit();
    
    Figure curr_figure = Figure::DIAMOND;
    InitDiamond();

    bool use_unif = false;

    while (window.isOpen()) {
        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::Resized) { glViewport(0, 0, event.size.width, event.size.height); }
            if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                    case sf::Keyboard::Escape: window.close(); break;
                    case sf::Keyboard::U:
                        use_unif = !use_unif;
                        Release();
                        if (curr_figure == DIAMOND) InitDiamond(use_unif);
                        else if (curr_figure == FAN) InitFan(use_unif);
                        else if (curr_figure == PENTAGON) InitPentagon(use_unif);
                        break;
                    case sf::Keyboard::A: 
                        Release();
                        InitDiamond(use_unif);
                        curr_figure = Figure::DIAMOND;    
                        break;
                    case sf::Keyboard::S: 
                        Release();
                        InitFan(use_unif);
                        curr_figure = Figure::FAN;     
                        break;
                    case sf::Keyboard::D: 
                        Release();
                        InitPentagon(use_unif);
                        curr_figure = Figure::PENTAGON;   
                        break;

                    case sf::Keyboard::Q:
                        Release();
                        InitFadeShader();      // Грузим новые шейдеры
                        InitVBODiamondFade();  // Грузим данные с цветами
                        curr_figure = Figure::FADE_DIAMOND; // Придумай новый enum
                        break;
                    case sf::Keyboard::W:
                        Release();
                        InitFadeShader();      // Грузим новые шейдеры
                        InitVBOFanFade();  // Грузим данные с цветами
                        curr_figure = Figure::FADE_FAN; // Придумай новый enum
                        break;
                    case sf::Keyboard::E:
                        Release();
                        InitFadeShader();      // Грузим новые шейдеры
                        InitVBOPentagonFade();  // Грузим данные с цветами
                        curr_figure = Figure::FADE_PENTAGON; // Придумай новый enum
                        break;
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (curr_figure == Figure::DIAMOND) {
            DrawDiamond();
        } else if (curr_figure == Figure::FAN) {
            DrawFan();
        } else if (curr_figure == Figure::PENTAGON) {
            DrawPentagon();
        } else if (curr_figure == Figure::FADE_DIAMOND) {
            DrawFadeDiamond();
        } else if (curr_figure == Figure::FADE_FAN) {
            DrawFadeFan();
        } else if (curr_figure == Figure::FADE_PENTAGON) {
            DrawFadePentagon();
        }

        window.display();
    }
    Release();
    return 0;
}
