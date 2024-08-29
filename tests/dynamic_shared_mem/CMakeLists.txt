add_executable(
        test_dynamic_shared_mem
        main.cpp
)
target_link_libraries(
        test_dynamic_shared_mem
        PRIVATE
        driverapi
)

configure_file("${CMAKE_CURRENT_LIST_DIR}/write_float.cubin" ${CMAKE_BINARY_DIR}/tests/write_float COPYONLY)