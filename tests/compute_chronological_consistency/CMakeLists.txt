add_executable(
        test_chronological_consistency
        main.cpp
)
target_link_libraries(
        test_chronological_consistency
        PRIVATE
        driverapi
)

configure_file("${CMAKE_CURRENT_LIST_DIR}/write_float.cubin" ${CMAKE_BINARY_DIR}/tests/compute_chronological_consistency COPYONLY)