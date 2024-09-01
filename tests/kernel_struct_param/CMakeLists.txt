add_executable(
        test_kernel_struct_param
        main.cpp
)
target_link_libraries(
        test_kernel_struct_param
        PRIVATE
        driverapi
)

configure_file("${CMAKE_CURRENT_LIST_DIR}/read_from_struct.cubin" ${CMAKE_BINARY_DIR}/tests/kernel_struct_param COPYONLY)