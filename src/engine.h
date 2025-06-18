#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace deep
{
    #pragma region Data

        struct RenderContext
        {
            SDL_Window* window;
            SDL_GPUDevice* device;
            SDL_GPUGraphicsPipeline* graphicsPipeline;

            SDL_GPUTexture* diffuseMap;
            SDL_GPUTexture* specularMap;
            SDL_GPUTexture* shininessMap;
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

        struct Light {
            glm::vec3 position;
            float padding1;
        
            glm::vec3 ambient;
            float padding2;
            glm::vec3 diffuse;
            float padding3;
            glm::vec3 specular;
            float padding4;

            glm::vec3 constantLinearQuadratic;
            float padding5;
        };

        struct FragmentUniformBuffer
        {
            glm::vec3 cameraPosition;
            float padding1;
            Light lights[10];
            int numberOfLights;
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

        struct Lights
        {
            glm::vec3 data[10];
            int count = 0;
        };

    #pragma endregion Data

    #pragma region Assets
        SDL_Surface* Load_Image(const char* imageFilename, int desiredChannels)
        {
            char fullPath[256];
            SDL_Surface *result;
            SDL_PixelFormat format;

            SDL_snprintf(fullPath, sizeof(fullPath), "ressources/%s", imageFilename);

            result = SDL_LoadBMP(fullPath);
            if (result == NULL)
            {
                SDL_Log("Failed to load BMP: %s", SDL_GetError());
                return NULL;
            }

            if (desiredChannels == 4)
            {
                format = SDL_PIXELFORMAT_ABGR8888;
            }
            else
            {
                SDL_assert(!"Unexpected desiredChannels");
                SDL_DestroySurface(result);
                return NULL;
            }
            if (result->format != format)
            {
                SDL_Surface *next = SDL_ConvertSurface(result, format);
                SDL_DestroySurface(result);
                result = next;
            }

            return result;
        }
    #pragma endregion Assets

    #pragma region Camera
        void CameraUpdateVectors(Camera& camera)
        {
            glm::vec3 front;
            front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            front.y = sin(glm::radians(camera.pitch));
            front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            camera.front = glm::normalize(front);
            
            camera.right = glm::normalize(glm::cross(camera.front, camera.worldUp));
            camera.up    = glm::normalize(glm::cross(camera.right, camera.front));
        }

        void CameraInit(Camera& camera, glm::vec3 position)
        {
            camera.position = position;
            camera.worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
            camera.yaw = -90.0f;
            camera.pitch = 0.0f;
            CameraUpdateVectors(camera);

            camera.movementSpeed = 2.5f;
            camera.mouseSensitivity = 0.1f;

            camera.projection = glm::perspective(glm::radians(45.0f), 640.0f / 480.0f, 0.1f, 100.0f);
        }

        glm::mat4 CameraGetViewMatrix(Camera& camera)
        {
            return glm::lookAt(camera.position, camera.position + camera.front, camera.up);
        }

        void CameraProcessKeyboard(Camera& camera, bool forward, bool back, bool left, bool right, bool up, bool down, float deltaTime)
        {
            if(forward || back || left || right || up || down)
            {
                glm::vec3 movement = glm::vec3(0.0f, 0.0f, 0.0f);
                if(forward || back || left || right)
                {
                    if(forward) { movement += camera.front; }
                    if(back) { movement -= camera.front; }
                    if(left) { movement -= camera.right; }
                    if(right) { movement += camera.right; }
                    movement.y = 0.0f;
                    movement = glm::normalize(movement);
                }

                if(up) { movement.y += 1.0f; }
                if(down) { movement.y -= 1.0f; }

                float velocity = camera.movementSpeed * deltaTime;
                camera.position += movement * velocity;
            }
        }

        void CameraProcessMouseMovement(Camera& camera, float xoffset, float yoffset, bool constrainPitch)
        {
            xoffset *= camera.mouseSensitivity;
            yoffset *= camera.mouseSensitivity;

            camera.yaw   += xoffset;
            camera.pitch += yoffset;

            if (constrainPitch)
            {
                if (camera.pitch > 89.0f)
                    camera.pitch = 89.0f;
                if (camera.pitch < -89.0f)
                    camera.pitch = -89.0f;
            }

            CameraUpdateVectors(camera);
        }

        
    #pragma endregion Camera

    #pragma region Renderer
        void Create_Window(RenderContext& renderContext)
        {
            // create a window
            renderContext.window = SDL_CreateWindow("Deep", 640, 480, SDL_WINDOW_RESIZABLE);
            
            // create the device
            renderContext.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
            SDL_ClaimWindowForGPUDevice(renderContext.device, renderContext.window);
        }
        void Destroy_Window(RenderContext& renderContext)
        {
            // destroy the GPU device
            SDL_DestroyGPUDevice(renderContext.device);

            // destroy the window
            SDL_DestroyWindow(renderContext.window);
        }

        void Create_Render_Pipeline(RenderContext& renderContext)
        {
        // load the vertex shader code
            size_t vertexCodeSize; 
            void* vertexCode = SDL_LoadFile("shaders/vertex.spv", &vertexCodeSize);

            // create the vertex shader
            SDL_GPUShaderCreateInfo vertexInfo{};
            vertexInfo.code = (Uint8*)vertexCode;
            vertexInfo.code_size = vertexCodeSize;
            vertexInfo.entrypoint = "main";
            vertexInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
            vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
            vertexInfo.num_samplers = 0;
            vertexInfo.num_storage_buffers = 0;
            vertexInfo.num_storage_textures = 0;
            vertexInfo.num_uniform_buffers = 1;

            SDL_GPUShader* vertexShader = SDL_CreateGPUShader(renderContext.device, &vertexInfo);

            // free the file
            SDL_free(vertexCode);

            // load the fragment shader code
            size_t fragmentCodeSize; 
            void* fragmentCode = SDL_LoadFile("shaders/fragment.spv", &fragmentCodeSize);

            // create the fragment shader
            SDL_GPUShaderCreateInfo fragmentInfo{};
            fragmentInfo.code = (Uint8*)fragmentCode;
            fragmentInfo.code_size = fragmentCodeSize;
            fragmentInfo.entrypoint = "main";
            fragmentInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
            fragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
            fragmentInfo.num_samplers = 3;
            fragmentInfo.num_storage_buffers = 0;
            fragmentInfo.num_storage_textures = 0;
            fragmentInfo.num_uniform_buffers = 1;

            SDL_GPUShader* fragmentShader = SDL_CreateGPUShader(renderContext.device, &fragmentInfo);

            // free the file
            SDL_free(fragmentCode);

            // create the graphics pipeline
            SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.vertex_shader = vertexShader;
            pipelineInfo.fragment_shader = fragmentShader;
            pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

            // describe the vertex buffers
            SDL_GPUVertexBufferDescription vertexBufferDesctiptions[1];
            vertexBufferDesctiptions[0].slot = 0;
            vertexBufferDesctiptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
            vertexBufferDesctiptions[0].instance_step_rate = 0;
            vertexBufferDesctiptions[0].pitch = sizeof(Vertex);

            pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
            pipelineInfo.vertex_input_state.vertex_buffer_descriptions = vertexBufferDesctiptions;

            // describe the vertex attribute
            SDL_GPUVertexAttribute vertexAttributes[3];

            // a_position
            vertexAttributes[0].buffer_slot = 0;
            vertexAttributes[0].location = 0;
            vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            vertexAttributes[0].offset = 0;

            // a_texcoord
            vertexAttributes[1].buffer_slot = 0;
            vertexAttributes[1].location = 1;
            vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            vertexAttributes[1].offset = sizeof(float) * 3;

            // a_normal
            vertexAttributes[2].buffer_slot = 0;
            vertexAttributes[2].location = 2;
            vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            vertexAttributes[2].offset = sizeof(float) * 5;

            pipelineInfo.vertex_input_state.num_vertex_attributes = 3;
            pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

            // describe the color target
            SDL_GPUColorTargetDescription colorTargetDescriptions[1];
            colorTargetDescriptions[0] = {};
            colorTargetDescriptions[0].blend_state.enable_blend = true;
            colorTargetDescriptions[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
            colorTargetDescriptions[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
            colorTargetDescriptions[0].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
            colorTargetDescriptions[0].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            colorTargetDescriptions[0].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
            colorTargetDescriptions[0].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            colorTargetDescriptions[0].format = SDL_GetGPUSwapchainTextureFormat(renderContext.device, renderContext.window);

            pipelineInfo.target_info.num_color_targets = 1;
            pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;

            // Cull Mode
            pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;

            // Depth Testing
            pipelineInfo.depth_stencil_state.enable_depth_test = true;
            pipelineInfo.depth_stencil_state.enable_depth_write = true;
            pipelineInfo.depth_stencil_state.compare_op = SDL_GPUCompareOp::SDL_GPU_COMPAREOP_LESS;
            pipelineInfo.target_info.has_depth_stencil_target = true;
            pipelineInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

            // create the pipeline
            renderContext.graphicsPipeline = SDL_CreateGPUGraphicsPipeline(renderContext.device, &pipelineInfo);

            // we don't need to store the shaders after creating the pipeline
            SDL_ReleaseGPUShader(renderContext.device, vertexShader);
            SDL_ReleaseGPUShader(renderContext.device, fragmentShader);
        }
        void Destroy_Render_Pipeline(RenderContext& renderContext)
        {
            // release the pipeline
            SDL_ReleaseGPUGraphicsPipeline(renderContext.device, renderContext.graphicsPipeline);        
        }

        void Create_Depth_Buffer(RenderContext& renderContext)
        {
            int sceneWidth, sceneHeight;
            SDL_GetWindowSizeInPixels(renderContext.window, &sceneWidth, &sceneHeight);

            SDL_GPUTextureCreateInfo textureCreateInfo{};
            textureCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
            textureCreateInfo.width = sceneWidth;
            textureCreateInfo.height = sceneHeight;
            textureCreateInfo.layer_count_or_depth = 1;
            textureCreateInfo.num_levels = 1;
            textureCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
            textureCreateInfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
            textureCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
            renderContext.sceneDepthTexture = SDL_CreateGPUTexture(renderContext.device, &textureCreateInfo);
        }
        void Destroy_Depth_Buffer(RenderContext& renderContext)
        {
            SDL_ReleaseGPUTexture(renderContext.device, renderContext.sceneDepthTexture);
        }

        SDL_GPUTexture* Load_Texture(RenderContext& renderContext, const char *filename)
        {
            SDL_Surface *imageData = Load_Image(filename, 4);
                if (imageData == NULL)
                {
                    SDL_Log("Could not load image data!");
                    // FIXME handle image not found
                }

            SDL_GPUSamplerCreateInfo samplerCreateInfo{};
            samplerCreateInfo.min_filter = SDL_GPU_FILTER_NEAREST;
            samplerCreateInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
            samplerCreateInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
            samplerCreateInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            samplerCreateInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            samplerCreateInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            renderContext.sampler = SDL_CreateGPUSampler(renderContext.device, &samplerCreateInfo);

            SDL_GPUTextureCreateInfo textureCreateInfo{};
            textureCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
            textureCreateInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
            textureCreateInfo.width = imageData->w;
            textureCreateInfo.height = imageData->h;
            textureCreateInfo.layer_count_or_depth = 1;
            textureCreateInfo.num_levels = 1;
            textureCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
            SDL_GPUTexture* texture = SDL_CreateGPUTexture(renderContext.device, &textureCreateInfo);

            SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo{};
            transferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transferBufferCreateInfo.size = imageData->w * imageData->h * 4;
            SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(
                renderContext.device,
                &transferBufferCreateInfo
            );

            auto textureTransferPtr = SDL_MapGPUTransferBuffer(
                renderContext.device,
                textureTransferBuffer,
                false
            );
            SDL_memcpy(textureTransferPtr, imageData->pixels, imageData->w * imageData->h * 4);
            SDL_UnmapGPUTransferBuffer(renderContext.device, textureTransferBuffer);

            // start a copy pass
            SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(renderContext.device);
            SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

            SDL_GPUTextureTransferInfo textureTransferInfo{};
            textureTransferInfo.transfer_buffer = textureTransferBuffer;
            textureTransferInfo.offset = 0; /* Zeros out the rest */
            SDL_GPUTextureRegion textureRegion{};
            textureRegion.texture = texture;
            textureRegion.w = imageData->w;
            textureRegion.h = imageData->h;
            textureRegion.d = 1;
            SDL_UploadToGPUTexture(
                copyPass,
                &textureTransferInfo,
                &textureRegion,
                false
            );

            SDL_EndGPUCopyPass(copyPass);
            SDL_SubmitGPUCommandBuffer(commandBuffer);
            SDL_DestroySurface(imageData);
            SDL_ReleaseGPUTransferBuffer(renderContext.device, textureTransferBuffer);

            return texture;
        }
        void Load_Textures(RenderContext& renderContext)
        {
            renderContext.diffuseMap = Load_Texture(renderContext, "diffuse.bmp");
            renderContext.specularMap = Load_Texture(renderContext, "specular.bmp");
            renderContext.shininessMap = Load_Texture(renderContext, "shininess.bmp");
        }
        void Destroy_Textures(RenderContext& renderContext)
        {
            // release the texture and sampler
            SDL_ReleaseGPUTexture(renderContext.device, renderContext.diffuseMap);
            SDL_ReleaseGPUTexture(renderContext.device, renderContext.specularMap);
            SDL_ReleaseGPUTexture(renderContext.device, renderContext.shininessMap);
            SDL_ReleaseGPUSampler(renderContext.device, renderContext.sampler);
        }

        void Create_Render_Data(RenderContext& renderContext, RenderData& renderData)
        {
            Vertex vertices[]
            {
                // Front face (Z = 0.5)
                // Normal: (0.0f, 0.0f, 1.0f) - points along positive Z-axis
                {-0.5f, -0.5f,  0.5f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f}, // 0
                { 0.5f, -0.5f,  0.5f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f}, // 1
                { 0.5f,  0.5f,  0.5f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f}, // 2
                {-0.5f,  0.5f,  0.5f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f}, // 3

                // Back face (Z = -0.5)
                // Normal: (0.0f, 0.0f, -1.0f) - points along negative Z-axis
                {-0.5f, -0.5f, -0.5f, 1.0f, 0.0f,  0.0f, 0.0f, -1.0f}, // 4
                { 0.5f, -0.5f, -0.5f, 0.0f, 0.0f,  0.0f, 0.0f, -1.0f}, // 5
                { 0.5f,  0.5f, -0.5f, 0.0f, 1.0f,  0.0f, 0.0f, -1.0f}, // 6
                {-0.5f,  0.5f, -0.5f, 1.0f, 1.0f,  0.0f, 0.0f, -1.0f}, // 7

                // Top face (Y = 0.5)
                // Normal: (0.0f, 1.0f, 0.0f) - points along positive Y-axis
                {-0.5f,  0.5f,  0.5f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f}, // 8 (same pos as 3)
                { 0.5f,  0.5f,  0.5f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f}, // 9 (same pos as 2)
                { 0.5f,  0.5f, -0.5f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f}, // 10 (same pos as 6)
                {-0.5f,  0.5f, -0.5f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f}, // 11 (same pos as 7)

                // Bottom face (Y = -0.5)
                // Normal: (0.0f, -1.0f, 0.0f) - points along negative Y-axis
                {-0.5f, -0.5f,  0.5f, 0.0f, 0.0f,  0.0f, -1.0f, 0.0f}, // 12 (same pos as 0)
                { 0.5f, -0.5f,  0.5f, 1.0f, 0.0f,  0.0f, -1.0f, 0.0f}, // 13 (same pos as 1)
                { 0.5f, -0.5f, -0.5f, 1.0f, 1.0f,  0.0f, -1.0f, 0.0f}, // 14 (same pos as 5)
                {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f,  0.0f, -1.0f, 0.0f}, // 15 (same pos as 4)

                // Right face (X = 0.5)
                // Normal: (1.0f, 0.0f, 0.0f) - points along positive X-axis
                { 0.5f, -0.5f,  0.5f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f}, // 16 (same pos as 1)
                { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f,  1.0f, 0.0f, 0.0f}, // 17 (same pos as 5)
                { 0.5f,  0.5f, -0.5f, 1.0f, 1.0f,  1.0f, 0.0f, 0.0f}, // 18 (same pos as 6)
                { 0.5f,  0.5f,  0.5f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f}, // 19 (same pos as 2)

                // Left face (X = -0.5)
                // Normal: (-1.0f, 0.0f, 0.0f) - points along negative X-axis
                {-0.5f, -0.5f,  0.5f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f}, // 20 (same pos as 0)
                {-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f}, // 21 (same pos as 4)
                {-0.5f,  0.5f, -0.5f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f}, // 22 (same pos as 7)
                {-0.5f,  0.5f,  0.5f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f}  // 23 (same pos as 3)
            };
            Uint16 indices[] = {  
                // Front face
                0, 1, 2,  2, 3, 0,

                // Back face
                4, 7, 6,  6, 5, 4,

                // Top face
                8, 9, 10,  10, 11, 8,

                // Bottom face
                12, 15, 14,  14, 13, 12,

                // Right face
                16, 17, 18,  18, 19, 16,

                // Left face
                20, 23, 22,  22, 21, 20,
            };
            Uint16 verticesCount = sizeof(vertices) / sizeof(Vertex);

            // create the vertex buffer
            SDL_GPUBufferCreateInfo vertexBufferInfo{};
            vertexBufferInfo.size = sizeof(vertices);
            vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            renderData.vertexBuffer = SDL_CreateGPUBuffer(renderContext.device, &vertexBufferInfo);

            // create the index buffer
            SDL_GPUBufferCreateInfo indexBufferInfo{};
            indexBufferInfo.size = sizeof(indices);
            indexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
            renderData.indexBuffer = SDL_CreateGPUBuffer(renderContext.device, &indexBufferInfo);

            // create a transfer buffer to upload to the vertex buffer
            SDL_GPUTransferBufferCreateInfo transferInfo{};
            transferInfo.size = sizeof(vertices) + sizeof(indices);
            transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(renderContext.device, &transferInfo);

            // fill the transfer buffer
            Vertex* transferData = (Vertex*)SDL_MapGPUTransferBuffer(renderContext.device, bufferTransferBuffer, false);
            SDL_memcpy(transferData, (void*)vertices, sizeof(vertices));
            Uint16* indexData = (Uint16*) &transferData[verticesCount];
            SDL_memcpy(indexData, (void*)indices, sizeof(indices));

            SDL_UnmapGPUTransferBuffer(renderContext.device, bufferTransferBuffer);

            // start a copy pass
            SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(renderContext.device);
            SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

            // upload the vertex buffer
            SDL_GPUTransferBufferLocation vertexBufferlocation{};
            vertexBufferlocation.transfer_buffer = bufferTransferBuffer;
            vertexBufferlocation.offset = 0;
            SDL_GPUBufferRegion vertexRegion{};
            vertexRegion.buffer = renderData.vertexBuffer;
            vertexRegion.size = sizeof(vertices);
            vertexRegion.offset = 0;
            SDL_UploadToGPUBuffer(copyPass, &vertexBufferlocation, &vertexRegion, false);

            // upload the index buffer
            SDL_GPUTransferBufferLocation indexBufferlocation{};
            indexBufferlocation.transfer_buffer = bufferTransferBuffer;
            indexBufferlocation.offset = sizeof(vertices);
            SDL_GPUBufferRegion indexRegion{};
            indexRegion.buffer = renderData.indexBuffer;
            indexRegion.size = sizeof(indices);
            indexRegion.offset = 0;
            SDL_UploadToGPUBuffer(copyPass, &indexBufferlocation, &indexRegion, false);

            // end the copy pass
            SDL_EndGPUCopyPass(copyPass);
            SDL_SubmitGPUCommandBuffer(commandBuffer);
            SDL_ReleaseGPUTransferBuffer(renderContext.device, bufferTransferBuffer);

            renderData.transform = glm::mat4(1.0f);
            renderData.transform = glm::rotate(renderData.transform, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        void Destroy_Render_Data(RenderContext& renderContext, RenderData& renderData){
            // release buffers
            SDL_ReleaseGPUBuffer(renderContext.device, renderData.vertexBuffer);
            SDL_ReleaseGPUBuffer(renderContext.device, renderData.indexBuffer);
        }

        void Render(RenderContext& renderContext, Camera& camera, Meshes& meshes, Lights& lights)
        {
            // acquire the command buffer
            SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(renderContext.device);

            // get the swapchain texture
            SDL_GPUTexture* swapchainTexture;
            Uint32 width, height;
            SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, renderContext.window, &swapchainTexture, &width, &height);

            // end the frame early if a swapchain texture is not available
            if (swapchainTexture == NULL)
            {
                // you must always submit the command buffer
                SDL_SubmitGPUCommandBuffer(commandBuffer);
                return;
            }

            // create the color target
            SDL_GPUColorTargetInfo colorTargetInfo{};
            colorTargetInfo.clear_color = {0/255.0f, 0/255.0f, 0/255.0f, 255/255.0f};
            colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
            colorTargetInfo.texture = swapchainTexture;

            SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo = {};
            depthStencilTargetInfo.texture = renderContext.sceneDepthTexture;
            depthStencilTargetInfo.cycle = true;
            depthStencilTargetInfo.clear_depth = 1;
            depthStencilTargetInfo.clear_stencil = 0;
            depthStencilTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            depthStencilTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
            depthStencilTargetInfo.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
            depthStencilTargetInfo.stencil_store_op = SDL_GPU_STOREOP_STORE;

                // begin a render pass
                SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, &depthStencilTargetInfo);

                // bind the pipeline
                SDL_BindGPUGraphicsPipeline(renderPass, renderContext.graphicsPipeline);

                VertexUniformBuffer vertexUniformBuffer{};
                vertexUniformBuffer.view = CameraGetViewMatrix(camera);
                vertexUniformBuffer.projection = camera.projection;

                FragmentUniformBuffer fragmentUniformBuffer{};

                for (int i = 0; i < lights.count; ++i) {
                    fragmentUniformBuffer.lights[i].position = lights.data[i];
                    fragmentUniformBuffer.lights[i].ambient = glm::vec3(0.2f, 0.2f, 0.2f);
                    fragmentUniformBuffer.lights[i].diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
                    fragmentUniformBuffer.lights[i].specular = glm::vec3(1.0f, 1.0f, 1.0f);
                    fragmentUniformBuffer.lights[i].constantLinearQuadratic = glm::vec3(1.0f, 0.09f, 0.032f);
                }
                fragmentUniformBuffer.numberOfLights = lights.count;
                fragmentUniformBuffer.cameraPosition = camera.position;
                SDL_PushGPUFragmentUniformData(commandBuffer, 0, &fragmentUniformBuffer, sizeof(FragmentUniformBuffer));

                SDL_GPUTextureSamplerBinding textureSamplerBinding[4];
                textureSamplerBinding[0].texture = renderContext.diffuseMap;
                textureSamplerBinding[0].sampler = renderContext.sampler;
                textureSamplerBinding[1].texture = renderContext.specularMap;
                textureSamplerBinding[1].sampler = renderContext.sampler;
                textureSamplerBinding[2].texture = renderContext.shininessMap;
                textureSamplerBinding[2].sampler = renderContext.sampler;
                textureSamplerBinding[3].texture = renderContext.sceneDepthTexture;
                textureSamplerBinding[3].sampler = renderContext.sampler;
                SDL_BindGPUFragmentSamplers(renderPass, 0, textureSamplerBinding, 3);
                for (int i = 0; i < meshes.count; ++i) {
                    vertexUniformBuffer.model = meshes.data[i].transform;
                    SDL_PushGPUVertexUniformData(commandBuffer, 0, &vertexUniformBuffer, sizeof(VertexUniformBuffer));

                    // bind the vertex buffer
                    SDL_GPUBufferBinding vertexBufferBinding{};
                    vertexBufferBinding.buffer = meshes.data[i].vertexBuffer;
                    vertexBufferBinding.offset = 0;
                    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

                    SDL_GPUBufferBinding indexBufferBinding{};
                    indexBufferBinding.buffer = meshes.data[i].indexBuffer;
                    indexBufferBinding.offset = 0;
                    SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

                    // issue a draw call
                    SDL_DrawGPUIndexedPrimitives(renderPass, 36, 1, 0, 0, 0);
                }

                // end the render pass
                SDL_EndGPURenderPass(renderPass);

            // submit the command buffer
            SDL_SubmitGPUCommandBuffer(commandBuffer);
        }
    #pragma endregion Renderer

    #pragma region Game
    void Load_Meshes(RenderContext& renderContext, Meshes& meshes)
        {
            glm::vec3 cubePositions[] = {
                glm::vec3( 0.0f,  0.0f,  0.0f), 
                glm::vec3( 2.0f,  5.0f, -15.0f), 
                glm::vec3(-1.5f, -2.2f, -2.5f),  
                glm::vec3(-3.8f, -2.0f, -12.3f),  
                glm::vec3( 2.4f, -0.4f, -3.5f),  
                glm::vec3(-1.7f,  3.0f, -7.5f),  
                glm::vec3( 1.3f, -2.0f, -2.5f),  
                glm::vec3( 1.5f,  2.0f, -2.5f), 
                glm::vec3( 1.5f,  0.2f, -1.5f), 
                glm::vec3(1.2f, 1.0f, 2.0f)  
            };

            for(unsigned int i = 0; i < 10; i++)
            {
                Create_Render_Data(renderContext, meshes.data[i]);

                meshes.data[i].transform = glm::mat4(1.0f);
                meshes.data[i].transform = glm::translate(meshes.data[i].transform, cubePositions[i]);
                float angle = 20.0f * i; 
                meshes.data[i].transform = glm::rotate(meshes.data[i].transform, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            }
            meshes.data[9].transform = glm::scale(meshes.data[9].transform, glm::vec3(0.1f));
            meshes.count = 10;
        }
        void Destroy_Meshes(RenderContext& renderContext, Meshes& meshes)
        {
            for (int i = 0; i < meshes.count; ++i) {
                Destroy_Render_Data(renderContext, meshes.data[i]);
            }
        }
        void Load_Lights(Lights& lights)
        {
            glm::vec3 pointLightPositions[] = {
                glm::vec3( 0.7f,  0.2f,  2.0f),
                glm::vec3( 2.3f, -3.3f, -4.0f),
                glm::vec3(-4.0f,  2.0f, -12.0f),
                glm::vec3( 0.0f,  0.0f, -3.0f)
            };  
            for(unsigned int i = 0; i < 4; i++)
            {
                lights.data[i] = pointLightPositions[i];
            }
            lights.count = 4;
        }
    #pragma endregion Game
} 