set(
    librecuda
    ${CMAKE_SOURCE_DIR}/src/librecuda.cpp
    ${CMAKE_SOURCE_DIR}/src/librecuda_status.cpp
    ${CMAKE_SOURCE_DIR}/src/cmdqueue.cpp
)

add_library(LibreCuda SHARED ${librecuda})
target_link_libraries(LibreCuda PRIVATE elfio)
