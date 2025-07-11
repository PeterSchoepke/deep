cmake_minimum_required(VERSION 3.24)

set(GLM_ROOT_DIR ${CMAKE_SOURCE_DIR}/vendor/glm)
set(CGLTF_ROOT_DIR ${CMAKE_SOURCE_DIR}/vendor/cgltf)
set(IMGUI_ROOT_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui)
set(IMGUI_BACKENDS_ROOT_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui/backends)

# start Steam
    set(STEAMWORKS_SDK_PATH ${CMAKE_SOURCE_DIR}/vendor/steamworks/sdk)
    if(NOT EXISTS "${STEAMWORKS_SDK_PATH}/public/steam/steam_api.h")
        message(FATAL_ERROR "Steamworks SDK not found at: ${STEAMWORKS_SDK_PATH}. Please set STEAMWORKS_SDK_PATH or ensure the SDK is in the expected location.")
    endif()
    set(STEAM_API_LIB_NAME "steam_api64.lib")
    set(STEAM_REDIST_DIR "${STEAMWORKS_SDK_PATH}/redistributable_bin/win64")
    find_library(STEAM_API_LIBRARY
        NAMES ${STEAM_API_LIB_NAME}
        PATHS ${STEAM_REDIST_DIR}
        REQUIRED
    )
    message(STATUS "Found Steamworks API library: ${STEAM_API_LIBRARY}")
# end Steam

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
target_link_libraries(${PROJECT_NAME} PRIVATE vendor ${STEAM_API_LIBRARY})
target_include_directories(${PROJECT_NAME} PRIVATE ${GLM_ROOT_DIR} ${CGLTF_ROOT_DIR} ${IMGUI_ROOT_DIR}  ${IMGUI_BACKENDS_ROOT_DIR} ${STEAMWORKS_SDK_PATH}/public)


add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/vendor/steamworks/sdk/redistributable_bin/win64/steam_api64.dll
            ${CMAKE_SOURCE_DIR}/build/Debug
    COMMENT "Copying steam_api64.dll to ${CMAKE_CURRENT_BINARY_DIR}"
)