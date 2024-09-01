add_executable(
        test_async_kernels
        main.cpp
)
target_link_libraries(
        test_async_kernels
        PRIVATE
        driverapi
)

configure_file("${CMAKE_CURRENT_LIST_DIR}/write_float.cubin" ${CMAKE_BINARY_DIR}/tests/write_float COPYONLY)