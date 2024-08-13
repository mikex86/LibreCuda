cmake_minimum_required(VERSION 3.5)
project(LibreCuda CXX)

include_directories(
        ${CMAKE_SOURCE_DIR}/src
)

# create elf-parser library
add_subdirectory(third_party/ELFIO)
add_subdirectory(src)
add_subdirectory(tests)

# TODO: REMOVE WHEN CRUDE DEBUG MAIN GETS DELETED
file(COPY ${CMAKE_SOURCE_DIR}/write_float.cubin DESTINATION ${CMAKE_BINARY_DIR})