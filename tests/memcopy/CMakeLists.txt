add_executable(
        test_memcopy
        main.cpp
)
target_link_libraries(
        test_memcopy
        PRIVATE
        driverapi
)