cmake_minimum_required(VERSION 3.5)
project(LibreCuda CXX)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
if(COMPILER_SUPPORTS_CXX17)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
   add_definitions(-DCOMPILEDWITHC17)
   message(STATUS "Using flag -std=c++17.")
else()
   message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
endif()

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/third_party/ELFIO/elfio)
        message("-- ELFIO not found, fetching ELFIO...")
        execute_process(COMMAND git submodule update --init)
        add_subdirectory(third_party/ELFIO)
else ()
        message("-- ELFIO found.")
        add_subdirectory(third_party/ELFIO)
endif()

add_subdirectory(driverapi)

add_subdirectory(tests)