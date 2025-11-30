cmake_minimum_required(VERSION 3.10)
project(GreenTriangle)

set(CMAKE_CXX_STANDARD 11)

find_package(SFML 2.6 COMPONENTS window system graphics REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_executable(GreenTriangle main.cpp)

target_link_libraries(GreenTriangle 
    sfml-window
    sfml-system
    sfml-graphics
    OpenGL::GL
    GLEW::GLEW
)