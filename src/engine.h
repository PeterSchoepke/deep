#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace deep
{
    #pragma region Data

        struct Render_Context
        {
            SDL_Window* window;
            SDL_GPUDevice* device;
            SDL_GPUGraphicsPipeline* graphics_pipeline;

            SDL_GPUTexture* diffuse_map;
            SDL_GPUTexture* specular_map;
            SDL_GPUTexture* shininess_map;
            SDL_GPUSampler* sampler;
            SDL_GPUTexture* scene_depth_texture;
        };

        struct Render_Data
        {
            SDL_GPUBuffer* vertex_buffer;
            SDL_GPUBuffer* index_buffer;
            glm::mat4 transform;
        };

        struct Vertex
        {
            float x, y, z;      //vec3 position
            float r, g, b, a;   //vec4 color
            float nx, ny, nz;   //vec3 normals
        };

        struct Vertex_Uniform_Buffer
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

            glm::vec3 constant_linear_quadratic;
            float padding5;
        };

        struct Fragment_Uniform_Buffer
        {
            glm::vec3 camera_position;
            float padding1;
            Light lights[10];
            int number_of_lights;
        };

        struct Camera
        {
            glm::vec3 position;
            glm::vec3 front;
            glm::vec3 up;
            glm::vec3 right;
            glm::vec3 world_up;
            
            float yaw;
            float pitch;
            
            float movement_speed;
            float mouse_sensitivity;
            
            glm::mat4 projection;
        };

        struct Meshes
        {
            Render_Data data[10];
            int count = 0;
        };

        struct Lights
        {
            glm::vec3 data[10];
            int count = 0;
        };

    #pragma endregion Data

    #pragma region Assets
        SDL_Surface* load_image(const char* image_filename, int desired_channels)
        {
            char full_path[256];
            SDL_Surface *result;
            SDL_PixelFormat format;

            SDL_snprintf(full_path, sizeof(full_path), "ressources/images/%s", image_filename);

            result = SDL_LoadBMP(full_path);
            if (result == NULL)
            {
                SDL_Log("Failed to load BMP: %s", SDL_GetError());
                return NULL;
            }

            if (desired_channels == 4)
            {
                format = SDL_PIXELFORMAT_ABGR8888;
            }
            else
            {
                SDL_assert(!"Unexpected desired_channels");
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
        void camera_update_vectors(Camera& camera)
        {
            glm::vec3 front;
            front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            front.y = sin(glm::radians(camera.pitch));
            front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            camera.front = glm::normalize(front);
            
            camera.right = glm::normalize(glm::cross(camera.front, camera.world_up));
            camera.up    = glm::normalize(glm::cross(camera.right, camera.front));
        }

        void camera_init(Camera& camera, glm::vec3 position)
        {
            camera.position = position;
            camera.world_up = glm::vec3(0.0f, 1.0f, 0.0f);
            camera.yaw = -90.0f;
            camera.pitch = 0.0f;
            camera_update_vectors(camera);

            camera.movement_speed = 2.5f;
            camera.mouse_sensitivity = 0.1f;

            camera.projection = glm::perspective(glm::radians(45.0f), 640.0f / 480.0f, 0.1f, 100.0f);
        }

        glm::mat4 camera_get_view_matrix(Camera& camera)
        {
            return glm::lookAt(camera.position, camera.position + camera.front, camera.up);
        }

        void camera_process_keyboard(Camera& camera, bool forward, bool back, bool left, bool right, bool up, bool down, float delta_time)
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

                float velocity = camera.movement_speed * delta_time;
                camera.position += movement * velocity;
            }
        }

        void camera_process_mouse_movement(Camera& camera, float x_offset, float y_offset, bool constrain_pitch)
        {
            x_offset *= camera.mouse_sensitivity;
            y_offset *= camera.mouse_sensitivity;

            camera.yaw   += x_offset;
            camera.pitch += y_offset;

            if (constrain_pitch)
            {
                if (camera.pitch > 89.0f)
                    camera.pitch = 89.0f;
                if (camera.pitch < -89.0f)
                    camera.pitch = -89.0f;
            }

            camera_update_vectors(camera);
        }

        
    #pragma endregion Camera

    #pragma region Renderer
        void create_window(Render_Context& render_context)
        {
            // create a window
            render_context.window = SDL_CreateWindow("Deep", 640, 480, SDL_WINDOW_RESIZABLE);
            
            // create the device
            render_context.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
            SDL_ClaimWindowForGPUDevice(render_context.device, render_context.window);
        }
        void destroy_window(Render_Context& render_context)
        {
            // destroy the GPU device
            SDL_DestroyGPUDevice(render_context.device);

            // destroy the window
            SDL_DestroyWindow(render_context.window);
        }

        void create_render_pipeline(Render_Context& render_context)
        {
        // load the vertex shader code
            size_t vertex_code_size; 
            void* vertex_code = SDL_LoadFile("shaders/vertex.spv", &vertex_code_size);

            // create the vertex shader
            SDL_GPUShaderCreateInfo vertex_info{};
            vertex_info.code = (Uint8*)vertex_code;
            vertex_info.code_size = vertex_code_size;
            vertex_info.entrypoint = "main";
            vertex_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
            vertex_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
            vertex_info.num_samplers = 0;
            vertex_info.num_storage_buffers = 0;
            vertex_info.num_storage_textures = 0;
            vertex_info.num_uniform_buffers = 1;

            SDL_GPUShader* vertex_shader = SDL_CreateGPUShader(render_context.device, &vertex_info);

            // free the file
            SDL_free(vertex_code);

            // load the fragment shader code
            size_t fragment_code_size; 
            void* fragment_code = SDL_LoadFile("shaders/fragment.spv", &fragment_code_size);

            // create the fragment shader
            SDL_GPUShaderCreateInfo fragment_info{};
            fragment_info.code = (Uint8*)fragment_code;
            fragment_info.code_size = fragment_code_size;
            fragment_info.entrypoint = "main";
            fragment_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
            fragment_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
            fragment_info.num_samplers = 3;
            fragment_info.num_storage_buffers = 0;
            fragment_info.num_storage_textures = 0;
            fragment_info.num_uniform_buffers = 1;

            SDL_GPUShader* fragment_shader = SDL_CreateGPUShader(render_context.device, &fragment_info);

            // free the file
            SDL_free(fragment_code);

            // create the graphics pipeline
            SDL_GPUGraphicsPipelineCreateInfo pipeline_info{};
            pipeline_info.vertex_shader = vertex_shader;
            pipeline_info.fragment_shader = fragment_shader;
            pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

            // describe the vertex buffers
            SDL_GPUVertexBufferDescription vertex_buffer_description[1];
            vertex_buffer_description[0].slot = 0;
            vertex_buffer_description[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
            vertex_buffer_description[0].instance_step_rate = 0;
            vertex_buffer_description[0].pitch = sizeof(Vertex);

            pipeline_info.vertex_input_state.num_vertex_buffers = 1;
            pipeline_info.vertex_input_state.vertex_buffer_descriptions = vertex_buffer_description;

            // describe the vertex attribute
            SDL_GPUVertexAttribute vertex_attributes[3];

            // a_position
            vertex_attributes[0].buffer_slot = 0;
            vertex_attributes[0].location = 0;
            vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            vertex_attributes[0].offset = 0;

            // a_texcoord
            vertex_attributes[1].buffer_slot = 0;
            vertex_attributes[1].location = 1;
            vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            vertex_attributes[1].offset = sizeof(float) * 3;

            // a_normal
            vertex_attributes[2].buffer_slot = 0;
            vertex_attributes[2].location = 2;
            vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            vertex_attributes[2].offset = sizeof(float) * 5;

            pipeline_info.vertex_input_state.num_vertex_attributes = 3;
            pipeline_info.vertex_input_state.vertex_attributes = vertex_attributes;

            // describe the color target
            SDL_GPUColorTargetDescription color_target_description[1];
            color_target_description[0] = {};
            color_target_description[0].blend_state.enable_blend = true;
            color_target_description[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
            color_target_description[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
            color_target_description[0].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
            color_target_description[0].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            color_target_description[0].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
            color_target_description[0].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            color_target_description[0].format = SDL_GetGPUSwapchainTextureFormat(render_context.device, render_context.window);

            pipeline_info.target_info.num_color_targets = 1;
            pipeline_info.target_info.color_target_descriptions = color_target_description;

            // Cull Mode
            pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;

            // Depth Testing
            pipeline_info.depth_stencil_state.enable_depth_test = true;
            pipeline_info.depth_stencil_state.enable_depth_write = true;
            pipeline_info.depth_stencil_state.compare_op = SDL_GPUCompareOp::SDL_GPU_COMPAREOP_LESS;
            pipeline_info.target_info.has_depth_stencil_target = true;
            pipeline_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

            // create the pipeline
            render_context.graphics_pipeline = SDL_CreateGPUGraphicsPipeline(render_context.device, &pipeline_info);

            // we don't need to store the shaders after creating the pipeline
            SDL_ReleaseGPUShader(render_context.device, vertex_shader);
            SDL_ReleaseGPUShader(render_context.device, fragment_shader);
        }
        void destroy_render_pipeline(Render_Context& render_context)
        {
            // release the pipeline
            SDL_ReleaseGPUGraphicsPipeline(render_context.device, render_context.graphics_pipeline);        
        }

        void create_depth_buffer(Render_Context& render_context)
        {
            int scene_width, scene_height;
            SDL_GetWindowSizeInPixels(render_context.window, &scene_width, &scene_height);

            SDL_GPUTextureCreateInfo texture_create_info{};
            texture_create_info.type = SDL_GPU_TEXTURETYPE_2D;
            texture_create_info.width = scene_width;
            texture_create_info.height = scene_height;
            texture_create_info.layer_count_or_depth = 1;
            texture_create_info.num_levels = 1;
            texture_create_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
            texture_create_info.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
            texture_create_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
            render_context.scene_depth_texture = SDL_CreateGPUTexture(render_context.device, &texture_create_info);
        }
        void destroy_depth_buffer(Render_Context& render_context)
        {
            SDL_ReleaseGPUTexture(render_context.device, render_context.scene_depth_texture);
        }

        SDL_GPUTexture* load_texture(Render_Context& render_context, const char *filename)
        {
            SDL_Surface *image_data = load_image(filename, 4);
                if (image_data == NULL)
                {
                    SDL_Log("Could not load image data!");
                    // FIXME handle image not found
                }

            SDL_GPUSamplerCreateInfo sampler_create_info{};
            sampler_create_info.min_filter = SDL_GPU_FILTER_NEAREST;
            sampler_create_info.mag_filter = SDL_GPU_FILTER_NEAREST;
            sampler_create_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
            sampler_create_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            sampler_create_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            sampler_create_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            render_context.sampler = SDL_CreateGPUSampler(render_context.device, &sampler_create_info);

            SDL_GPUTextureCreateInfo texture_create_info{};
            texture_create_info.type = SDL_GPU_TEXTURETYPE_2D;
            texture_create_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
            texture_create_info.width = image_data->w;
            texture_create_info.height = image_data->h;
            texture_create_info.layer_count_or_depth = 1;
            texture_create_info.num_levels = 1;
            texture_create_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
            SDL_GPUTexture* texture = SDL_CreateGPUTexture(render_context.device, &texture_create_info);

            SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{};
            transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transfer_buffer_create_info.size = image_data->w * image_data->h * 4;
            SDL_GPUTransferBuffer* texture_transfer_buffer = SDL_CreateGPUTransferBuffer(
                render_context.device,
                &transfer_buffer_create_info
            );

            auto texture_transfer_pointer = SDL_MapGPUTransferBuffer(
                render_context.device,
                texture_transfer_buffer,
                false
            );
            SDL_memcpy(texture_transfer_pointer, image_data->pixels, image_data->w * image_data->h * 4);
            SDL_UnmapGPUTransferBuffer(render_context.device, texture_transfer_buffer);

            // start a copy pass
            SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(render_context.device);
            SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

            SDL_GPUTextureTransferInfo texture_transfer_info{};
            texture_transfer_info.transfer_buffer = texture_transfer_buffer;
            texture_transfer_info.offset = 0; /* Zeros out the rest */
            SDL_GPUTextureRegion texture_region{};
            texture_region.texture = texture;
            texture_region.w = image_data->w;
            texture_region.h = image_data->h;
            texture_region.d = 1;
            SDL_UploadToGPUTexture(
                copy_pass,
                &texture_transfer_info,
                &texture_region,
                false
            );

            SDL_EndGPUCopyPass(copy_pass);
            SDL_SubmitGPUCommandBuffer(command_buffer);
            SDL_DestroySurface(image_data);
            SDL_ReleaseGPUTransferBuffer(render_context.device, texture_transfer_buffer);

            return texture;
        }
        void load_textures(Render_Context& render_context)
        {
            render_context.diffuse_map = load_texture(render_context, "diffuse.bmp");
            render_context.specular_map = load_texture(render_context, "specular.bmp");
            render_context.shininess_map = load_texture(render_context, "shininess.bmp");
        }
        void destroy_textures(Render_Context& render_context)
        {
            // release the texture and sampler
            SDL_ReleaseGPUTexture(render_context.device, render_context.diffuse_map);
            SDL_ReleaseGPUTexture(render_context.device, render_context.specular_map);
            SDL_ReleaseGPUTexture(render_context.device, render_context.shininess_map);
            SDL_ReleaseGPUSampler(render_context.device, render_context.sampler);
        }

        void create_render_data(Render_Context& render_context, Render_Data& render_data)
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
            Uint16 vertices_count = sizeof(vertices) / sizeof(Vertex);

            // create the vertex buffer
            SDL_GPUBufferCreateInfo vertex_buffer_info{};
            vertex_buffer_info.size = sizeof(vertices);
            vertex_buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            render_data.vertex_buffer = SDL_CreateGPUBuffer(render_context.device, &vertex_buffer_info);

            // create the index buffer
            SDL_GPUBufferCreateInfo index_buffer_info{};
            index_buffer_info.size = sizeof(indices);
            index_buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
            render_data.index_buffer = SDL_CreateGPUBuffer(render_context.device, &index_buffer_info);

            // create a transfer buffer to upload to the vertex buffer
            SDL_GPUTransferBufferCreateInfo transfer_info{};
            transfer_info.size = sizeof(vertices) + sizeof(indices);
            transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            SDL_GPUTransferBuffer* buffer_transfer_buffer = SDL_CreateGPUTransferBuffer(render_context.device, &transfer_info);

            // fill the transfer buffer
            Vertex* transfer_data = (Vertex*)SDL_MapGPUTransferBuffer(render_context.device, buffer_transfer_buffer, false);
            SDL_memcpy(transfer_data, (void*)vertices, sizeof(vertices));
            Uint16* index_data = (Uint16*) &transfer_data[vertices_count];
            SDL_memcpy(index_data, (void*)indices, sizeof(indices));

            SDL_UnmapGPUTransferBuffer(render_context.device, buffer_transfer_buffer);

            // start a copy pass
            SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(render_context.device);
            SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

            // upload the vertex buffer
            SDL_GPUTransferBufferLocation vertex_buffer_location{};
            vertex_buffer_location.transfer_buffer = buffer_transfer_buffer;
            vertex_buffer_location.offset = 0;
            SDL_GPUBufferRegion vertex_region{};
            vertex_region.buffer = render_data.vertex_buffer;
            vertex_region.size = sizeof(vertices);
            vertex_region.offset = 0;
            SDL_UploadToGPUBuffer(copy_pass, &vertex_buffer_location, &vertex_region, false);

            // upload the index buffer
            SDL_GPUTransferBufferLocation index_buffer_location{};
            index_buffer_location.transfer_buffer = buffer_transfer_buffer;
            index_buffer_location.offset = sizeof(vertices);
            SDL_GPUBufferRegion index_region{};
            index_region.buffer = render_data.index_buffer;
            index_region.size = sizeof(indices);
            index_region.offset = 0;
            SDL_UploadToGPUBuffer(copy_pass, &index_buffer_location, &index_region, false);

            // end the copy pass
            SDL_EndGPUCopyPass(copy_pass);
            SDL_SubmitGPUCommandBuffer(command_buffer);
            SDL_ReleaseGPUTransferBuffer(render_context.device, buffer_transfer_buffer);

            render_data.transform = glm::mat4(1.0f);
            render_data.transform = glm::rotate(render_data.transform, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        }
        void destroy_render_data(Render_Context& render_context, Render_Data& render_data){
            // release buffers
            SDL_ReleaseGPUBuffer(render_context.device, render_data.vertex_buffer);
            SDL_ReleaseGPUBuffer(render_context.device, render_data.index_buffer);
        }

        void render(Render_Context& render_context, Camera& camera, Meshes& meshes, Lights& lights)
        {
            // acquire the command buffer
            SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(render_context.device);

            // get the swapchain texture
            SDL_GPUTexture* swapchain_texture;
            Uint32 width, height;
            SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, render_context.window, &swapchain_texture, &width, &height);

            // end the frame early if a swapchain texture is not available
            if (swapchain_texture == NULL)
            {
                // you must always submit the command buffer
                SDL_SubmitGPUCommandBuffer(command_buffer);
                return;
            }

            // create the color target
            SDL_GPUColorTargetInfo color_target_info{};
            color_target_info.clear_color = {0/255.0f, 0/255.0f, 0/255.0f, 255/255.0f};
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;
            color_target_info.texture = swapchain_texture;

            SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {};
            depth_stencil_target_info.texture = render_context.scene_depth_texture;
            depth_stencil_target_info.cycle = true;
            depth_stencil_target_info.clear_depth = 1;
            depth_stencil_target_info.clear_stencil = 0;
            depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            depth_stencil_target_info.store_op = SDL_GPU_STOREOP_STORE;
            depth_stencil_target_info.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
            depth_stencil_target_info.stencil_store_op = SDL_GPU_STOREOP_STORE;

                // begin a render pass
                SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, &depth_stencil_target_info);

                // bind the pipeline
                SDL_BindGPUGraphicsPipeline(render_pass, render_context.graphics_pipeline);

                Vertex_Uniform_Buffer vertex_uniform_buffer{};
                vertex_uniform_buffer.view = camera_get_view_matrix(camera);
                vertex_uniform_buffer.projection = camera.projection;

                Fragment_Uniform_Buffer fragment_uniform_buffer{};

                for (int i = 0; i < lights.count; ++i) {
                    fragment_uniform_buffer.lights[i].position = lights.data[i];
                    fragment_uniform_buffer.lights[i].ambient = glm::vec3(0.2f, 0.2f, 0.2f);
                    fragment_uniform_buffer.lights[i].diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
                    fragment_uniform_buffer.lights[i].specular = glm::vec3(1.0f, 1.0f, 1.0f);
                    fragment_uniform_buffer.lights[i].constant_linear_quadratic = glm::vec3(1.0f, 0.09f, 0.032f);
                }
                fragment_uniform_buffer.number_of_lights = lights.count;
                fragment_uniform_buffer.camera_position = camera.position;
                SDL_PushGPUFragmentUniformData(command_buffer, 0, &fragment_uniform_buffer, sizeof(Fragment_Uniform_Buffer));

                SDL_GPUTextureSamplerBinding texture_sampler_binding[4];
                texture_sampler_binding[0].texture = render_context.diffuse_map;
                texture_sampler_binding[0].sampler = render_context.sampler;
                texture_sampler_binding[1].texture = render_context.specular_map;
                texture_sampler_binding[1].sampler = render_context.sampler;
                texture_sampler_binding[2].texture = render_context.shininess_map;
                texture_sampler_binding[2].sampler = render_context.sampler;
                texture_sampler_binding[3].texture = render_context.scene_depth_texture;
                texture_sampler_binding[3].sampler = render_context.sampler;
                SDL_BindGPUFragmentSamplers(render_pass, 0, texture_sampler_binding, 3);
                for (int i = 0; i < meshes.count; ++i) {
                    vertex_uniform_buffer.model = meshes.data[i].transform;
                    SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_uniform_buffer, sizeof(Vertex_Uniform_Buffer));

                    // bind the vertex buffer
                    SDL_GPUBufferBinding vertex_buffer_binding{};
                    vertex_buffer_binding.buffer = meshes.data[i].vertex_buffer;
                    vertex_buffer_binding.offset = 0;
                    SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);

                    SDL_GPUBufferBinding index_buffer_binding{};
                    index_buffer_binding.buffer = meshes.data[i].index_buffer;
                    index_buffer_binding.offset = 0;
                    SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

                    // issue a draw call
                    SDL_DrawGPUIndexedPrimitives(render_pass, 36, 1, 0, 0, 0);
                }

                // end the render pass
                SDL_EndGPURenderPass(render_pass);

            // submit the command buffer
            SDL_SubmitGPUCommandBuffer(command_buffer);
        }
    #pragma endregion Renderer

    #pragma region Game
        void destroy_meshes(Render_Context& render_context, Meshes& meshes)
        {
            for (int i = 0; i < meshes.count; ++i) {
                destroy_render_data(render_context, meshes.data[i]);
            }
        }

        void init(Render_Context& render_context)
        {
            create_window(render_context);
            create_render_pipeline(render_context);
            create_depth_buffer(render_context);
            load_textures(render_context);
        }
        void cleanup(Render_Context& render_context, Meshes& meshes)
        {
            destroy_meshes(render_context, meshes);
            destroy_textures(render_context);
            destroy_depth_buffer(render_context);
            destroy_render_pipeline(render_context);
            destroy_window(render_context);
        }
    
        void load_meshes(Render_Context& render_context, Meshes& meshes)
        {
            glm::vec3 cube_positions[] = {
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
                create_render_data(render_context, meshes.data[i]);

                meshes.data[i].transform = glm::mat4(1.0f);
                meshes.data[i].transform = glm::translate(meshes.data[i].transform, cube_positions[i]);
                float angle = 20.0f * i; 
                meshes.data[i].transform = glm::rotate(meshes.data[i].transform, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            }
            meshes.data[9].transform = glm::scale(meshes.data[9].transform, glm::vec3(0.1f));
            meshes.count = 10;
        }
        
        void load_lights(Lights& lights)
        {
            glm::vec3 light_positions[] = {
                glm::vec3( 0.7f,  0.2f,  2.0f),
                glm::vec3( 2.3f, -3.3f, -4.0f),
                glm::vec3(-4.0f,  2.0f, -12.0f),
                glm::vec3( 0.0f,  0.0f, -3.0f)
            };  
            for(unsigned int i = 0; i < 4; i++)
            {
                lights.data[i] = light_positions[i];
            }
            lights.count = 4;
        }
        
        #pragma endregion Game
} 