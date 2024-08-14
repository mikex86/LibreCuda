set(
    DRIVERAPI_SOURCES
    ${CMAKE_SOURCE_DIR}/driverapi/src/librecuda.cpp
    ${CMAKE_SOURCE_DIR}/driverapi/src/librecuda_status.cpp
    ${CMAKE_SOURCE_DIR}/driverapi/src/cmdqueue.cpp
)

add_library(driverapi SHARED ${DRIVERAPI_SOURCES})
target_include_directories(
    driverapi PUBLIC 
    ${CMAKE_SOURCE_DIR}/driverapi/include 
    ${CMAKE_SOURCE_DIR}/third_party/ELFIO 
    ${CMAKE_SOURCE_DIR}/driverapi/internal)
target_link_libraries(driverapi PRIVATE elfio)