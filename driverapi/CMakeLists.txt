set(
        DRIVERAPI_SOURCES
        src/librecuda.cpp
        src/librecuda_status.cpp
        src/cmdqueue.cpp
        src/memcopy.cpp
        internal/memcopy.h
        internal/memcopy_kernels.h
)
if (BUILD_LIBRECUDA_DRVIER_API_STATIC_LIB)
    add_library(driverapi STATIC ${DRIVERAPI_SOURCES})
else()
    add_library(driverapi SHARED ${DRIVERAPI_SOURCES})
endif()

target_include_directories(driverapi PRIVATE "internal/")
target_include_directories(driverapi PUBLIC "include/")
target_link_libraries(driverapi PRIVATE elfio)