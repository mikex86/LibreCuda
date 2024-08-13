add_executable(
    main
    ${CMAKE_SOURCE_DIR}/tests/main.cpp 
)

target_link_libraries(
    main 
    LibreCuda
)
