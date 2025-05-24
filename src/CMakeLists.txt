cmake_minimum_required(VERSION 3.24)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE 
    main.cpp
    render.h
    render.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE vendor)
