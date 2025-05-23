#pragma once

#include <SDL3/SDL.h>
#include "assets.h"

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

void DEEP_Create_Window(RenderContext& renderContext);
void DEEP_Destroy_Window(RenderContext& renderContext);

void DEEP_Create_Render_Pipeline(RenderContext& renderContext);
void DEEP_Destroy_Render_Pipeline(RenderContext& renderContext);

void DEEP_Create_Render_Data(RenderContext& renderContext, RenderData& renderData);
void DEEP_Destroy_Render_Data(RenderContext& renderContext, RenderData& renderData);

void DEEP_Render(RenderContext& renderContext, RenderData& renderData);