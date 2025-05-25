#pragma once

#include <SDL3/SDL.h>
#include "assets.h"

namespace deep
{
    typedef struct RenderContext
    {
        SDL_Window* window;
        SDL_GPUDevice* device;
        SDL_GPUGraphicsPipeline* graphicsPipeline;

        SDL_GPUTexture* texture;
        SDL_GPUSampler* sampler;
    } RenderContext;

    typedef struct RenderData
    {
        SDL_GPUBuffer* vertexBuffer;
        SDL_GPUBuffer* indexBuffer;
    } RenderData;

    struct Vertex
    {
        float x, y, z;      //vec3 position
        float r, g, b, a;   //vec4 color
    };

    void Create_Window(RenderContext& renderContext);
    void Destroy_Window(RenderContext& renderContext);

    void Create_Render_Pipeline(RenderContext& renderContext);
    void Destroy_Render_Pipeline(RenderContext& renderContext);

    void Create_Render_Data(RenderContext& renderContext, RenderData& renderData);
    void Destroy_Render_Data(RenderContext& renderContext, RenderData& renderData);

    void Render(RenderContext& renderContext, RenderData& renderData);
}