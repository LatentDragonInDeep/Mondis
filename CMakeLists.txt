cmake_minimum_required(VERSION 3.10)
project(Mondis)

set(CMAKE_CXX_STANDARD 17)
AUX_SOURCE_DIRECTORY(src SRC_DIRS)
add_compile_options(-w -o2)

add_executable(Mondis ${SRC_DIRS} src/MondisObject.cpp src/Main.cpp)