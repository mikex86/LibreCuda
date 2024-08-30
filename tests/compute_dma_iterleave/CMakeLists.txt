add_executable(
        test_compute_dma_interleave
        main.cpp
)
target_link_libraries(
        test_compute_dma_interleave
        PRIVATE
        driverapi
)

configure_file("${CMAKE_CURRENT_LIST_DIR}/write_float.cubin" ${CMAKE_BINARY_DIR}/tests/write_float COPYONLY)