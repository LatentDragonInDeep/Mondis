cmake_minimum_required(VERSION 3.10)
project(Mondis)

set(CMAKE_CXX_STANDARD 17)

AUX_SOURCE_DIRECTORY(src/ SRC_DIR)

add_executable(Mondis ${SRC_DIR} src/SkipList.cpp src/SkipList.h src/MondisBinary.cpp src/MondisBinary.h)