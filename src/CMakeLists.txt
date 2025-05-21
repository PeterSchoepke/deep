cmake_minimum_required(VERSION 3.24)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE 
    main.cpp
    core/game.cpp
    core/game.h
    core/window.cpp
    core/window.h
)
target_link_libraries(${PROJECT_NAME} PRIVATE vendor)
