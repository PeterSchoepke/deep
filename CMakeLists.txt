cmake_minimum_required(VERSION 3.15)
 

set(CMAKE_CXX_STANDARD 20)
project(game CXX)
 

include(FetchContent)
FetchContent_Declare(
    SDL2 
    GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
    GIT_TAG "origin/SDL2"
)
FetchContent_MakeAvailable(SDL2)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME} 
	"third_party/glad/glad.c" 
	
    "src/main.cpp"    
	
    "src/core/game.h"
    "src/core/game.cpp"
    "src/core/window.h"
    "src/core/window.cpp"
)
target_link_libraries(${PROJECT_NAME} SDL2::SDL2-static SDL2::SDL2main)
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR})