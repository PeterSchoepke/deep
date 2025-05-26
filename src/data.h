#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace deep
{
    struct RenderContext
    {
        SDL_Window* window;
        SDL_GPUDevice* device;
        SDL_GPUGraphicsPipeline* graphicsPipeline;

        SDL_GPUTexture* texture;
        SDL_GPUSampler* sampler;
        SDL_GPUTexture* sceneDepthTexture;
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

    struct Camera
    {
        glm::mat4 view;
        glm::mat4 projection;
    };

    struct Meshes
    {
        RenderData data[10];
        int count = 0;
    };
}