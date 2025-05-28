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
        float nx, ny, nz;   //vec3 normals
    };

    struct VertexUniformBuffer
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    };

    struct FragmentUniformBuffer
    {
        glm::vec3 lightColor;
        float padding1;
        glm::vec3 lightPosition;
        float padding2;
        glm::vec3 cameraPosition;
    };

    struct Camera
    {
        glm::vec3 position;
        glm::vec3 front;
        glm::vec3 up;
        glm::vec3 right;
        glm::vec3 worldUp;
        
        float yaw;
        float pitch;
        
        float movementSpeed;
        float mouseSensitivity;
        
        glm::mat4 projection;
    };

    struct Meshes
    {
        RenderData data[10];
        int count = 0;
    };
}