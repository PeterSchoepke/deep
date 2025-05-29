#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "data.h"
#include "render.h"

namespace deep
{
    SDL_Surface* Load_Image(const char* imageFilename, int desiredChannels);

    void Load_Meshes(RenderContext& renderContext, Meshes& meshes);
    void Destroy_Meshes(RenderContext& renderContext, Meshes& meshes);
    void Load_Lights(Lights& lights);
} 