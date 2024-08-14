set(
    DRIVERAPI_SOURCES
    ${CMAKE_SOURCE_DIR}/driverapi/librecuda.cpp
    ${CMAKE_SOURCE_DIR}/driverapi/librecuda_status.cpp
    ${CMAKE_SOURCE_DIR}/driverapi/cmdqueue.cpp
)


add_library(driverapi SHARED ${DRIVERAPI_SOURCES})
target_include_directories(driverapi PUBLIC ${CMAKE_SOURCE_DIR}/driverapi/include ${CMAKE_SOURCE_DIR}/driverapi/internal)
target_link_libraries(driverapi PRIVATE elfio)