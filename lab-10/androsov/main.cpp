#include <SFML/Window.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <iostream>
#include <vector>
#include <string>

// ID шейдерной программы
GLuint Program;
// ID вершинного атрибута
GLint Attrib_vertex;
// ID Vertex Buffer Object
GLuint VBO;

//структура вершины
struct Vertex {
  GLfloat x;
  GLfloat y;
};

//Вершинный шейдер (просто передает позицию вершины)
const char* VertexShaderSource = R"(
  #version 330 core
  in vec2 coord;
  void main() {
    gl_Position = vec4(coord, 0.0, 1.0);
  }
)";

// фрагментный шейдер (выводит зеленый цвет)
const char* FragShaderSource = R"(
  #version 330 core
  out vec4 color;
  void main() {
    color = vec4(0, 1, 0, 1); //Зеленый цвет (R=0, G=1, B=0)
  }
)";

// --- ПРОТОТИПЫ ФУНКЦИЙ ---
void Init();
void Draw();
void Release();
void InitShader();
void InitVBO();
void ShaderLog(unsigned int shader);
void checkOpenGLerror(); 
void ReleaseVBO();
void ReleaseShader();

int main() {
sf::Window window(sf::VideoMode(600, 600), "My OpenGL window", sf::Style::Default, sf::ContextSettings(24));
  window.setVerticalSyncEnabled(true);
  window.setActive(true);
  
  glewInit();
  Init();
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) { window.close(); }
      else if (event.type == sf::Event::Resized) { glViewport(0, 0, event.size.width, event.size.height); }
    }
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Draw();
    window.display();
  }
  Release();
  return 0;
}
//Инициализируем ресурсы
void Init() {
  // Шейдеры
  InitShader();
  
  // Вершинный буфер
  InitVBO();
}

//Инициализируем буфер вершин

void InitVBO() {
  glGenBuffers(1, &VBO);
  // Вершины нашего треугольника
  Vertex triangle[3] = {
    { -1.0f, -1.0f },
    { 0.0f, 1.0f },
    { 1.0f, -1.0f }, 
  };
  //активизируем буфер
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  // Передаем вершины в буфер
  glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
  
    checkOpenGLerror(); //Пример функции есть в лабораторной
    // Проверка ошибок OpenGL, если есть, то вывод в консоль тип ошибки
}

//InitShader - инициализируем шейдеры
void InitShader() {
  // Создаем вершинный шейдер
  GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
  // Передаем исходный код
  glShaderSource(vShader, 1, &VertexShaderSource, NULL);
  // Компилируем шейдер
  glCompileShader(vShader);
  std::cout << "vertex shader \n";
  // Функция печати лога шейдера
  ShaderLog(vShader); 
  
  //Фрагментный шейдер
    //создаем его
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    //передаем исходных код
    glShaderSource(fShader, 1, &FragShaderSource, NULL);
    // Компилируем шейдер
    glCompileShader(fShader);
    std::cout << "fragment shader \n";
    // Функция печати лога шейдера
    ShaderLog(fShader); 
    
  // Шейдерная программа
  
   //Создаем программу 
   Program = glCreateProgram();
   //Прикрепляем к ней шейдеры
   glAttachShader(Program, vShader);
   glAttachShader(Program, fShader);
   
   //Линкуем шейдерную программу
   glLinkProgram(Program);
   
   //Проверяем линковку
   int link_ok;
   glGetProgramiv(Program, GL_LINK_STATUS, &link_ok);
   if (!link_ok) {
      std::cout << "error attach shaders \n";
   }
   
   
   //Устанавливаем связь между параметрами в проге и шейдере
   
    //Вытягиваем ID аттрибута из собранной программы
    const char* attr_name = "coord"; //имя в шейдере
    
    Attrib_vertex = glGetAttribLocation(Program, attr_name);
    
    if (Attrib_vertex == -1) {
        std::cout<<"could not bind attrib"<<attr_name<<std::endl;
        return;
    }
   checkOpenGLerror();
  }

//рисуем (наконец)
void Draw() {
   glUseProgram(Program); // Устанавливаем шейдерную программу текущей
   
   glEnableVertexAttribArray(Attrib_vertex); // Включаем массив атрибутов
   
   glBindBuffer(GL_ARRAY_BUFFER, VBO); //подключаем память видеокарты (VBO)
   
   // сообщаем OpenGL как он должен интерпретировать вершинные данные.
   glVertexAttribPointer(Attrib_vertex, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glBindBuffer(GL_ARRAY_BUFFER, 0); //Отключаем VBO
   
   glDrawArrays(GL_TRIANGLES, 0, 6);
   
    glDisableVertexAttribArray(Attrib_vertex); // Отключаем массив атрибутов
    glUseProgram(0); // Отключаем шейдерную программу
    
    checkOpenGLerror();
  }

//Функции для освобождения ресурсов
// Освобождение буфера
void ReleaseVBO() {
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &VBO);
}

//Освобождение шейдеров
void ReleaseShader() {
  // Передавая ноль, мы отключаем шейдерную программу
  glUseProgram(0);
  // Удаляем шейдерную программу
  glDeleteProgram(Program);
}
//общая функция
void Release() {
  // Шейдеры
  ReleaseShader();
  // Вершинный буфер
  ReleaseVBO();
}
   
   

//Обработка ошибок компиляции шейдеров
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
// Функция для проверки общих ошибок OpenGL
void checkOpenGLerror() {
    GLenum err = glGetError();
    while (err != GL_NO_ERROR) {
        std::string error;
        switch (err) {
            case GL_INVALID_ENUM:                  error = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "GL_INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                error = "GL_STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "GL_STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            default:                               error = "Unknown error"; break;
        }
        std::cerr << "[OpenGL Error] Type: " << error << " (Code: " << err << ")" << std::endl;
        err = glGetError(); // Проверяем следующую ошибку
    }
}
