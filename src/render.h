#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "assets.h"
#include "camera.h"

namespace deep
{
    struct RenderContext
    {
        SDL_Window* window;
        SDL_GPUDevice* device;
        SDL_GPUGraphicsPipeline* graphicsPipeline;

        SDL_GPUTexture* texture;
        SDL_GPUSampler* sampler;
    };

    struct RenderData
    {
        SDL_GPUBuffer* vertexBuffer;
        SDL_GPUBuffer* indexBuffer;
        glm::mat4 transform;
    };

    struct Vertex
    {
        float x, y, z;      //vec3 position
        float r, g, b, a;   //vec4 color
    };

    struct VertexUniformBuffer
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    };

    void Create_Window(RenderContext& renderContext);
    void Destroy_Window(RenderContext& renderContext);

    void Create_Render_Pipeline(RenderContext& renderContext);
    void Destroy_Render_Pipeline(RenderContext& renderContext);

    void Load_Textures(RenderContext& renderContext);
    void Destroy_Textures(RenderContext& renderContext);

    void Create_Render_Data(RenderContext& renderContext, RenderData& renderData);
    void Destroy_Render_Data(RenderContext& renderContext, RenderData& renderData);

    void Render(RenderContext& renderContext, Camera& camera, RenderData& renderData);
}