cmake_minimum_required(VERSION 3.24)

set(GLM_ROOT_DIR ${CMAKE_SOURCE_DIR}/vendor/glm)
set(CGLTF_ROOT_DIR ${CMAKE_SOURCE_DIR}/vendor/cgltf)
set(IMGUI_ROOT_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui)
set(IMGUI_BACKENDS_ROOT_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui/backends)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE 
    main.cpp
    ${IMGUI_ROOT_DIR}/imgui.cpp 
    ${IMGUI_ROOT_DIR}/imgui_demo.cpp 
    ${IMGUI_ROOT_DIR}/imgui_draw.cpp 
    ${IMGUI_ROOT_DIR}/imgui_tables.cpp 
    ${IMGUI_ROOT_DIR}/imgui_widgets.cpp
    ${IMGUI_BACKENDS_ROOT_DIR}/imgui_impl_sdl3.cpp
    ${IMGUI_BACKENDS_ROOT_DIR}/imgui_impl_sdlgpu3.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE vendor)
target_include_directories(${PROJECT_NAME} PRIVATE ${GLM_ROOT_DIR} ${CGLTF_ROOT_DIR} ${IMGUI_ROOT_DIR}  ${IMGUI_BACKENDS_ROOT_DIR})
