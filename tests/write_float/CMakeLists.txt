add_executable(
        test_write_float
        main.cpp
)
target_link_libraries(
        test_write_float
        PRIVATE
        driverapi
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/write_float.cubin ${CMAKE_BINARY_DIR}/bin COPYONLY)