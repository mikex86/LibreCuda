cmake_minimum_required(VERSION 3.5)
project(LibreCuda CXX)

# create elf-parser library
add_subdirectory(third_party/ELFIO)

add_executable(LibreCuda
        src/main.cpp
        src/librecuda.cpp
        src/librecuda_status.cpp
        src/cmdqueue.cpp)

target_link_libraries(LibreCuda PRIVATE elfio)