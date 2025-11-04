cmake_minimum_required(VERSION 3.10)
project(Lab7)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_executable(lab7 main.cpp)
target_link_libraries(lab7 ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

