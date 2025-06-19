# deep
A First Person Action Dungeon Crawler

## Setup (Windows)
### Programs
- Odin https://odin-lang.org/docs/install/
  - Add to Path
- VSCode https://code.visualstudio.com/download
  - Extension: Odin Language
- Vulkan SDK (Only for Shader Compilation) https://vulkan.lunarg.com/sdk/home#windows
### Project
- git clone https://github.com/PeterSchoepke/deep.git
- Press F5 in VSCode
  - if you just want to run the game you can use run.bat
### Compile Shaders (Vulkan)
- glslc -fshader-stage=vertex shaders/vertex.glsl -o shaders/vertex.spv
- glslc -fshader-stage=fragment shaders/fragment.glsl -o shaders/fragment.spv