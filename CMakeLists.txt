cmake_minimum_required(VERSION 3.5)
project(LibreCuda CXX)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

include_directories(
        ${CMAKE_SOURCE_DIR}/src
)

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/third_party/ELFIO/elfio)
        message("-- ELFIO not found, fetching ELFIO...")
        execute_process(COMMAND git submodule update --init)
else ()
        message("-- ELFIO found.")
        add_subdirectory(third_party/ELFIO)
endif()

# create elf-parser library
add_subdirectory(src)
add_subdirectory(tests)

# TODO: REMOVE WHEN CRUDE DEBUG MAIN GETS DELETED
file(COPY ${CMAKE_SOURCE_DIR}/write_float.cubin DESTINATION ${CMAKE_BINARY_DIR}/bin)