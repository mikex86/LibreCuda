cmake_minimum_required(VERSION 3.5)
project(LibreCuda CXX)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )


if(NOT EXISTS ${CMAKE_SOURCE_DIR}/third_party/ELFIO/elfio)
        message("-- ELFIO not found, fetching ELFIO...")
        execute_process(COMMAND git submodule update --init)
else ()
        message("-- ELFIO found.")
        add_subdirectory(third_party/ELFIO)
endif()

set(
    DRIVERAPI_SOURCES
    ${CMAKE_SOURCE_DIR}/src/librecuda.cpp
    ${CMAKE_SOURCE_DIR}/src/librecuda_status.cpp
    ${CMAKE_SOURCE_DIR}/src/cmdqueue.cpp
)

add_library(driverapi SHARED ${DRIVERAPI_SOURCES})
target_include_directories(driverapi PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(driverapi PRIVATE elfio)

add_subdirectory(tests)