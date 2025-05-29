#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "assets.h"
#include "data.h"
#include "camera.h"

namespace deep
{   
    void Create_Window(RenderContext& renderContext);
    void Destroy_Window(RenderContext& renderContext);

    void Create_Render_Pipeline(RenderContext& renderContext);
    void Destroy_Render_Pipeline(RenderContext& renderContext);

    void Create_Depth_Buffer(RenderContext& renderContext);
    void Destroy_Depth_Buffer(RenderContext& renderContext);

    void Load_Textures(RenderContext& renderContext);
    SDL_GPUTexture* Load_Texture(RenderContext& renderContext, const char *filename);
    void Destroy_Textures(RenderContext& renderContext);

    void Create_Render_Data(RenderContext& renderContext, RenderData& renderData);
    void Destroy_Render_Data(RenderContext& renderContext, RenderData& renderData);

    void Render(RenderContext& renderContext, Camera& camera, Meshes& meshes, Lights& lights);
}