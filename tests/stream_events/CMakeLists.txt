add_executable(
        test_stream_events
        main.cpp
)
target_link_libraries(
        test_stream_events
        PRIVATE
        driverapi
)

configure_file("${CMAKE_CURRENT_LIST_DIR}/write_float.cubin" ${CMAKE_BINARY_DIR}/tests/test_stream_events COPYONLY)