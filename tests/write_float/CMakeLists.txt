add_executable(
    test_write_float
    main.cpp 
)
target_link_libraries(
    test_write_float 
    PRIVATE
    driverapi
)

file(COPY ${CMAKE_SOURCE_DIR}/tests/write_float/write_float.cubin DESTINATION ${CMAKE_BINARY_DIR}/bin)