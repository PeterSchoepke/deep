cmake_minimum_required(VERSION 3.24)

set(GLM_ROOT_DIR ${CMAKE_SOURCE_DIR}/vendor/glm)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE 
    main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE vendor)
target_include_directories(${PROJECT_NAME} PRIVATE ${GLM_ROOT_DIR})
