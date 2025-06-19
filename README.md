# deep
A First Person Action Dungeon Crawler

## Setup (Windows)
### Programs
- Visual Studio https://visualstudio.microsoft.com/de/vs/community/
- CMake https://cmake.org/download/
- VSCode https://code.visualstudio.com/download
  - Extension: C/C++
  - Extension: CMake Tools
  - Extension: Shader languaes support for VS Code
- Vulkan SDK (Only for Shader Compilation) https://vulkan.lunarg.com/sdk/home#windows
### Project
- git clone https://github.com/PeterSchoepke/deep.git
- git submodule update --remote
- mkdir build
- cmake -S . -B build
- MSBuild build\src\deep.vcxproj
- Press F5 in VSCode
### Compile Shaders (Vulkan)
- glslc -fshader-stage=vertex shaders/vertex.glsl -o shaders/vertex.spv
- glslc -fshader-stage=fragment shaders/fragment.glsl -o shaders/fragment.spv