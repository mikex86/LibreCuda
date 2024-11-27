add_executable(
        many_kernels_launch
        main.cpp
)
target_link_libraries(
        many_kernels_launch
        PRIVATE
        driverapi
)

configure_file("${CMAKE_CURRENT_LIST_DIR}/empty_kernel.cubin" ${CMAKE_BINARY_DIR}/tests/many_kernels_launch COPYONLY)