cmake_minimum_required(VERSION 3.10)
project(Mondis)

set(CMAKE_CXX_STANDARD 17)
AUX_SOURCE_DIRECTORY(src SRC_DIRS)
add_compile_options(-w -o2 -D_GLIBCXX_HAS_GTHREADS -D_GLIBCXX_USE_C99_STDINT_TR1)
set(CMAKE_EXE_LINKER_FLAGS -static)

add_executable(Mondis ${SRC_DIRS})