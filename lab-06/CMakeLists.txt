cmake_minimum_required(VERSION 3.10)
project(Lab6)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_executable(lab6 main.cpp)
target_link_libraries(lab6 ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
