set(
    DRIVERAPI_SOURCES
    src/librecuda.cpp
    src/librecuda_status.cpp
    src/cmdqueue.cpp
)

add_library(driverapi SHARED ${DRIVERAPI_SOURCES})
target_include_directories(driverapi PRIVATE "internal/")
target_include_directories(driverapi PUBLIC "include/")
target_link_libraries(driverapi PRIVATE elfio)