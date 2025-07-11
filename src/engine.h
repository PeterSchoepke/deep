#pragma once

#define CGLTF_IMPLEMENTATION

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cgltf.h>
#include <steam/steam_api.h>

namespace deep
{
    const int MAP_SIZE_X = 35;
    const int MAP_SIZE_Y = 15;
    
    struct Entity
    {
        int id;
        bool is_active = false;
        glm::mat4 transform = glm::mat4(1.0f);

        bool light_component = false;
        glm::vec3 light_position = glm::vec3(0.0f, 0.0f, 0.0f);

        bool mesh_component = false;
        SDL_GPUBuffer* vertex_buffer;
        SDL_GPUBuffer* index_buffer;
        int index_count;

        bool hurt_component = false;
        float collision_radius = 0.5f;
        float sight = 14.0f;
        float speed = 3.0f;

        bool exit_component = false;
    };
}

namespace deepcore
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

        struct Vertex
        {
            float position[3];
            float texcoord[2];
            float normal[3];
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

        struct Sound {
            Uint8 *wav_data;
            Uint32 wav_data_len;
            SDL_AudioStream *stream;
        };

        struct Sound_System
        {
            Sound data[10];
            Sound music;
            int max_count = 10;
            int count = 0;
            SDL_AudioDeviceID audio_device = 0;
        };

        struct Entities
        {
            deep::Entity data[10];
            int max_count = 10;
            int count = 0;
        };

        struct Map_Mesh
        {
            bool has_mesh = false;
            SDL_GPUBuffer* vertex_buffer;
            SDL_GPUBuffer* index_buffer;
            int index_count;

            bool is_collision_top = false;
            bool is_collision_right = false;
            bool is_collision_bottom = false;
            bool is_collision_left = false;
            bool has_any_collision = false;
        };

        struct Map
        {
            Map_Mesh meshes[10];
            int meshes_max_count = 10;
            int meshes_count = 0;

            int map[deep::MAP_SIZE_Y][deep::MAP_SIZE_X] = {};
        };
    #pragma endregion Data

    #pragma region Globals
        Render_Context render_context{};
        Entities entities{};
        Sound_System sound_system{};
        Camera camera{};
        Map map{};
        bool steam_init = false;
    #pragma endregion Globals

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
        void camera_update_vectors()
        {
            glm::vec3 front;
            front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            front.y = sin(glm::radians(camera.pitch));
            front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            camera.front = glm::normalize(front);
            
            camera.right = glm::normalize(glm::cross(camera.front, camera.world_up));
            camera.up    = glm::normalize(glm::cross(camera.right, camera.front));
        }

        void camera_init(glm::vec3 position)
        {
            camera.position = position;
            camera.world_up = glm::vec3(0.0f, 1.0f, 0.0f);
            camera.yaw = -90.0f;
            camera.pitch = 0.0f;
            camera_update_vectors();

            camera.movement_speed = 2.5f;
            camera.mouse_sensitivity = 0.1f;

            camera.projection = glm::perspective(glm::radians(45.0f), 640.0f / 480.0f, 0.1f, 100.0f);
        }

        glm::vec2 camera_get_position_2d()
        {
            return glm::vec2(camera.position.x, camera.position.z);
        }

        void camera_set_position(glm::vec3 position)
        {
            camera.position = position;
        }

        glm::mat4 camera_get_view_matrix()
        {
            return glm::lookAt(camera.position, camera.position + camera.front, camera.up);
        }

        bool is_position_blocked(glm::vec3 current_position, glm::vec3 next_position, float radius);
        void camera_process_keyboard(bool forward, bool back, bool left, bool right, bool up, bool down, float delta_time)
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
                    if (glm::length(movement) > 0.0001f) {
                        movement = glm::normalize(movement);
                    }
                }

                if(up) { movement.y += 1.0f; }
                if(down) { movement.y -= 1.0f; }
                if (glm::length(movement) > 0.0001 && !(forward || back || left || right)) {
                    movement = glm::normalize(movement);
                }

                float velocity = camera.movement_speed * delta_time;
                glm::vec3 desired_position_change = movement * velocity;

                float camera_collision_radius = 0.3f;

                glm::vec3 original_position = camera.position;
                glm::vec3 temp_position = camera.position;

                temp_position.x = original_position.x + desired_position_change.x;
                if (!is_position_blocked(camera.position, temp_position, camera_collision_radius))
                {
                    camera.position.x = temp_position.x;
                }
                else
                {
                    temp_position.x = original_position.x;
                }

                temp_position.z = original_position.z + desired_position_change.z;
                if (!is_position_blocked(camera.position, glm::vec3(camera.position.x, temp_position.y, temp_position.z), camera_collision_radius))
                {
                    camera.position.z = temp_position.z;
                }
                else
                {
                    temp_position.z = original_position.z;
                }

                camera.position.y += desired_position_change.y;
            }
        }

        void camera_process_mouse_movement(float x_offset, float y_offset, bool constrain_pitch)
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

            camera_update_vectors();
        } 
    #pragma endregion Camera

    #pragma region Renderer
        void create_window()
        {
            // create a window
            render_context.window = SDL_CreateWindow("Deep", 640, 480, SDL_WINDOW_RESIZABLE);
            
            // create the device
            render_context.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
            SDL_ClaimWindowForGPUDevice(render_context.device, render_context.window);
        }

        void create_render_pipeline()
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
            //pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;

            // Depth Testing
            pipeline_info.depth_stencil_state.enable_depth_test = true;
            pipeline_info.depth_stencil_state.enable_depth_write = true;
            pipeline_info.depth_stencil_state.compare_op = SDL_GPUCompareOp::SDL_GPU_COMPAREOP_LESS;
            pipeline_info.target_info.has_depth_stencil_target = true;
            pipeline_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;

            // create the pipeline
            render_context.graphics_pipeline = SDL_CreateGPUGraphicsPipeline(render_context.device, &pipeline_info);

            // we don't need to store the shaders after creating the pipeline
            SDL_ReleaseGPUShader(render_context.device, vertex_shader);
            SDL_ReleaseGPUShader(render_context.device, fragment_shader);
        }

        void create_depth_buffer()
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
            texture_create_info.format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;
            texture_create_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
            render_context.scene_depth_texture = SDL_CreateGPUTexture(render_context.device, &texture_create_info);
        }

        void init_sound()
        {
            sound_system.audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
            if (sound_system.audio_device == 0) {
                SDL_Log("Couldn't open audio device: %s", SDL_GetError());
            }
        }

        void setup_imgui()
        {
            float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
            //ImGui::StyleColorsLight();

            // Setup scaling
            ImGuiStyle& style = ImGui::GetStyle();
            style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
            style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

            // Setup Platform/Renderer backends
            ImGui_ImplSDL3_InitForSDLGPU(render_context.window);
            ImGui_ImplSDLGPU3_InitInfo init_info = {};
            init_info.Device = render_context.device;
            init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(render_context.device, render_context.window);
            init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
            ImGui_ImplSDLGPU3_Init(&init_info);

            // Start the Dear ImGui frame
            ImGui_ImplSDLGPU3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
        }

        SDL_GPUTexture* load_texture(const char *filename)
        {
            SDL_Surface *image_data = load_image(filename, 4);
                if (image_data == NULL)
                {
                    SDL_Log("Could not load image data!");
                    // FIXME handle image not found
                }          

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
        void load_textures()
        {
            SDL_GPUSamplerCreateInfo sampler_create_info{};
            sampler_create_info.min_filter = SDL_GPU_FILTER_NEAREST;
            sampler_create_info.mag_filter = SDL_GPU_FILTER_NEAREST;
            sampler_create_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
            sampler_create_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            sampler_create_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            sampler_create_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            render_context.sampler = SDL_CreateGPUSampler(render_context.device, &sampler_create_info);

            render_context.diffuse_map = load_texture("diffuse.bmp");
            render_context.specular_map = load_texture("specular.bmp");
            render_context.shininess_map = load_texture("shininess.bmp");
        }

        void create_render_data(deep::Entity& render_data, std::vector<Vertex>& vertices, std::vector<Uint16>& indices)
        {
            // create the vertex buffer
            SDL_GPUBufferCreateInfo vertex_buffer_info{};
            vertex_buffer_info.size = vertices.size() * sizeof(Vertex);
            vertex_buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            render_data.vertex_buffer = SDL_CreateGPUBuffer(render_context.device, &vertex_buffer_info);

            // create the index buffer
            SDL_GPUBufferCreateInfo index_buffer_info{};
            index_buffer_info.size = indices.size() * sizeof(Uint16);
            index_buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
            render_data.index_buffer = SDL_CreateGPUBuffer(render_context.device, &index_buffer_info);

            // create a transfer buffer to upload to the vertex buffer
            SDL_GPUTransferBufferCreateInfo transfer_info{};
            transfer_info.size = vertices.size() * sizeof(Vertex) + indices.size() * sizeof(Uint16);
            transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            SDL_GPUTransferBuffer* buffer_transfer_buffer = SDL_CreateGPUTransferBuffer(render_context.device, &transfer_info);

            // fill the transfer buffer
            Vertex* transfer_data = (Vertex*)SDL_MapGPUTransferBuffer(render_context.device, buffer_transfer_buffer, false);
            SDL_memcpy(transfer_data, vertices.data(), vertices.size() * sizeof(Vertex));
            Uint16* index_data = (Uint16*) &transfer_data[vertices.size()];
            SDL_memcpy(index_data, indices.data(), indices.size() * sizeof(Uint16));

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
            vertex_region.size = vertices.size() * sizeof(Vertex);
            vertex_region.offset = 0;
            SDL_UploadToGPUBuffer(copy_pass, &vertex_buffer_location, &vertex_region, false);

            // upload the index buffer
            SDL_GPUTransferBufferLocation index_buffer_location{};
            index_buffer_location.transfer_buffer = buffer_transfer_buffer;
            index_buffer_location.offset = vertices.size() * sizeof(Vertex);
            SDL_GPUBufferRegion index_region{};
            index_region.buffer = render_data.index_buffer;
            index_region.size = indices.size() * sizeof(Uint16);
            index_region.offset = 0;
            SDL_UploadToGPUBuffer(copy_pass, &index_buffer_location, &index_region, false);

            // end the copy pass
            SDL_EndGPUCopyPass(copy_pass);
            SDL_SubmitGPUCommandBuffer(command_buffer);
            SDL_ReleaseGPUTransferBuffer(render_context.device, buffer_transfer_buffer);

            render_data.index_count = indices.size();
            render_data.transform = glm::mat4(1.0f);
        }
        void destroy_render_data(deep::Entity& render_data){
            // release buffers
            SDL_ReleaseGPUBuffer(render_context.device, render_data.vertex_buffer);
            SDL_ReleaseGPUBuffer(render_context.device, render_data.index_buffer);
        }

        void load_gltf(const char *model_filename, deep::Entity& render_data)
        {
            cgltf_options options = {};
            cgltf_data* data = NULL;
            cgltf_result result = cgltf_parse_file(&options, model_filename, &data);

            if (result == cgltf_result_success)
            {
                result = cgltf_load_buffers(&options, data, model_filename);
                if (result == cgltf_result_success)
                {
                    if(data->meshes_count > 0)
                    {
                        cgltf_mesh* mesh = &data->meshes[0];

                        for(int i = 0; i < mesh->primitives_count; i++)
                        {
                            cgltf_primitive* primitive = &mesh->primitives[i];
                            if(primitive->type == cgltf_primitive_type_triangles)
                            {
                                std::vector<Vertex> vertices;
                                std::vector<Uint16> indices;

                                Uint16 current_vertex_offset = vertices.size();

                                const cgltf_accessor* pos_accessor = NULL;
                                const cgltf_accessor* normal_accessor = NULL;
                                const cgltf_accessor* texcoord_accessor = NULL;

                                for (size_t j = 0; j < primitive->attributes_count; ++j) {
                                    const cgltf_attribute* attribute = &primitive->attributes[j];
                                    if (attribute->type == cgltf_attribute_type_position) {
                                        pos_accessor = attribute->data;
                                    } else if (attribute->type == cgltf_attribute_type_normal) {
                                        normal_accessor = attribute->data;
                                    } else if (attribute->type == cgltf_attribute_type_texcoord) { // TEXCOORD_0
                                        texcoord_accessor = attribute->data;
                                    }
                                    // Add more attribute types (e.g., TANGENT, COLOR_0) as needed
                                }

                                if (pos_accessor) {
                                    vertices.reserve(vertices.size() + pos_accessor->count);

                                    for (cgltf_size k = 0; k < pos_accessor->count; ++k) {
                                        Vertex v = {};

                                        cgltf_accessor_read_float(pos_accessor, k, v.position, 3);

                                        if (normal_accessor) {
                                            cgltf_accessor_read_float(normal_accessor, k, v.normal, 3);
                                        } else {
                                            v.normal[0] = 0.0f; v.normal[1] = 0.0f; v.normal[2] = 0.0f;
                                        }

                                        if (texcoord_accessor) {
                                            cgltf_accessor_read_float(texcoord_accessor, k, v.texcoord, 2);
                                        } else {
                                            v.texcoord[0] = 0.0f; v.texcoord[1] = 0.0f;
                                        }
                                        vertices.push_back(v);
                                    }
                                } else {
                                    SDL_Log("Warning: Primitive found without position data.\n");
                                    continue;
                                }


                                if (primitive->indices != NULL) {
                                    const cgltf_accessor* indices_accessor = primitive->indices;

                                    indices.reserve(indices.size() + indices_accessor->count);

                                    for (cgltf_size k = 0; k < indices_accessor->count; ++k) {
                                        unsigned int index;
                                        cgltf_accessor_read_uint(indices_accessor, k, &index, 1);

                                        indices.push_back(current_vertex_offset + index);
                                    }
                                } else {
                                    if (pos_accessor->count % 3 != 0) {
                                        SDL_Log("Warning: Unindexed primitive has vertex count (%zu) not divisible by 3.\n", (size_t)pos_accessor->count);
                                    }

                                    indices.reserve(indices.size() + pos_accessor->count);
                                    for (Uint16 k = 0; k < pos_accessor->count; ++k) {
                                        indices.push_back(current_vertex_offset + k);
                                    }
                                }

                                create_render_data(render_data, vertices, indices);
                            }
                        }
                    }
                } else {
                    SDL_Log("Failed to load glTF buffers for %s", model_filename);
                }
                cgltf_free(data);
            } else {
                SDL_Log("Failed to parse glTF file %s", model_filename);
            }
        }

        void render()
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
                vertex_uniform_buffer.view = camera_get_view_matrix();
                vertex_uniform_buffer.projection = camera.projection;

                Fragment_Uniform_Buffer fragment_uniform_buffer{};

                int lights_count = 0;
                for (int i = 0; i < entities.count; ++i) {
                    if(entities.data[i].light_component)
                    {
                        fragment_uniform_buffer.lights[lights_count].position = entities.data[i].light_position;
                        fragment_uniform_buffer.lights[lights_count].ambient = glm::vec3(0.2f, 0.2f, 0.2f);
                        fragment_uniform_buffer.lights[lights_count].diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
                        fragment_uniform_buffer.lights[lights_count].specular = glm::vec3(1.0f, 1.0f, 1.0f);
                        fragment_uniform_buffer.lights[lights_count].constant_linear_quadratic = glm::vec3(1.0f, 0.09f, 0.032f);
                        lights_count++;
                    }
                }
                fragment_uniform_buffer.number_of_lights = lights_count;
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
                for (int i = 0; i < entities.count; ++i) {
                    if(entities.data[i].is_active && entities.data[i].mesh_component)
                    {
                        vertex_uniform_buffer.model = entities.data[i].transform;
                        SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_uniform_buffer, sizeof(Vertex_Uniform_Buffer));

                        // bind the vertex buffer
                        SDL_GPUBufferBinding vertex_buffer_binding{};
                        vertex_buffer_binding.buffer = entities.data[i].vertex_buffer;
                        vertex_buffer_binding.offset = 0;
                        SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);

                        SDL_GPUBufferBinding index_buffer_binding{};
                        index_buffer_binding.buffer = entities.data[i].index_buffer;
                        index_buffer_binding.offset = 0;
                        SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

                        // issue a draw call
                        SDL_DrawGPUIndexedPrimitives(render_pass, entities.data[i].index_count, 1, 0, 0, 0);
                    }
                }

                for (int row = 0; row < deep::MAP_SIZE_Y; ++row) {
                    for (int col = 0; col < deep::MAP_SIZE_X; ++col) {
                        if (map.map[row][col] != 0) 
                        {
                            int mesh_index = map.map[row][col] - 1;

                            glm::mat4 transform = glm::mat4(1.0f);
                            transform = glm::translate(transform, glm::vec3(col*3.0f+1.5f, 0.0f, row*3.0f+1.5f));

                            vertex_uniform_buffer.model = transform;
                            SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_uniform_buffer, sizeof(Vertex_Uniform_Buffer));

                            // bind the vertex buffer
                            SDL_GPUBufferBinding vertex_buffer_binding{};
                            vertex_buffer_binding.buffer = map.meshes[mesh_index].vertex_buffer;
                            vertex_buffer_binding.offset = 0;
                            SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);

                            SDL_GPUBufferBinding index_buffer_binding{};
                            index_buffer_binding.buffer = map.meshes[mesh_index].index_buffer;
                            index_buffer_binding.offset = 0;
                            SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

                            // issue a draw call
                            SDL_DrawGPUIndexedPrimitives(render_pass, map.meshes[mesh_index].index_count, 1, 0, 0, 0);
                        }
                    }
                }

                // end the render pass
                SDL_EndGPURenderPass(render_pass);

                // ImGui Rendering Pass (No Depth)
                ImGui::Render();
                ImDrawData* draw_data = ImGui::GetDrawData();
                const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
                ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);
                SDL_GPUColorTargetInfo imgui_color_target_info{};
                imgui_color_target_info.texture = swapchain_texture;
                imgui_color_target_info.load_op = SDL_GPU_LOADOP_LOAD;
                imgui_color_target_info.store_op = SDL_GPU_STOREOP_STORE;
                SDL_GPURenderPass* imgui_render_pass = SDL_BeginGPURenderPass(command_buffer, &imgui_color_target_info, 1, NULL); // NULL for depth_stencil_target_info
                if (!is_minimized)
                {
                    ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, imgui_render_pass);
                }
                SDL_EndGPURenderPass(imgui_render_pass);

            // submit the command buffer
            SDL_SubmitGPUCommandBuffer(command_buffer);

            // Start the Dear ImGui frame
            ImGui_ImplSDLGPU3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
        }
    #pragma endregion Renderer

    #pragma region Audio
        void load_music(const char *filename)
        {            
            SDL_AudioSpec spec;
            char *wav_path = NULL;

            /* Load the .wav files from wherever the app is being run from. */
            SDL_asprintf(&wav_path, "ressources/music/%s", filename);  /* allocate a string of the full file path */
            if (!SDL_LoadWAV(wav_path, &spec, &sound_system.music.wav_data, &sound_system.music.wav_data_len)) {
                SDL_Log("Couldn't load .wav file: %s", SDL_GetError());
            }

            /* Create an audio stream. Set the source format to the wav's format (what
            we'll input), leave the dest format NULL here (it'll change to what the
            device wants once we bind it). */
            sound_system.music.stream = SDL_CreateAudioStream(&spec, NULL);
            if (!sound_system.music.stream) {
                SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
            } else if (!SDL_BindAudioStream(sound_system.audio_device, sound_system.music.stream)) {  /* once bound, it'll start playing when there is data available! */
                SDL_Log("Failed to bind '%s' stream to device: %s", filename, SDL_GetError());
            }

            SDL_free(wav_path);  /* done with this string. */            
        }

        int load_sound(const char *filename)
        {
            if(sound_system.count < sound_system.max_count)
            {
                int i = sound_system.count;
                SDL_AudioSpec spec;
                char *wav_path = NULL;

                /* Load the .wav files from wherever the app is being run from. */
                SDL_asprintf(&wav_path, "ressources/sound/%s", filename);  /* allocate a string of the full file path */
                if (!SDL_LoadWAV(wav_path, &spec, &sound_system.data[i].wav_data, &sound_system.data[i].wav_data_len)) {
                    SDL_Log("Couldn't load .wav file: %s", SDL_GetError());
                }

                /* Create an audio stream. Set the source format to the wav's format (what
                we'll input), leave the dest format NULL here (it'll change to what the
                device wants once we bind it). */
                sound_system.data[i].stream = SDL_CreateAudioStream(&spec, NULL);
                if (!sound_system.data[i].stream) {
                    SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
                } else if (!SDL_BindAudioStream(sound_system.audio_device, sound_system.data[i].stream)) {  /* once bound, it'll start playing when there is data available! */
                    SDL_Log("Failed to bind '%s' stream to device: %s", filename, SDL_GetError());
                }

                SDL_free(wav_path);  /* done with this string. */
                
                sound_system.count += 1;
                return sound_system.count-1;
            }
            return -1;            
        }

        void play_sound(int id)
        {
            if (SDL_GetAudioStreamQueued(sound_system.data[id].stream) < (int)sound_system.data[id].wav_data_len) {
                /* feed more data to the stream. It will queue at the end, and trickle out as the hardware needs more data. */
                SDL_PutAudioStreamData(sound_system.data[id].stream, sound_system.data[id].wav_data, sound_system.data[id].wav_data_len);
            }
        }

        void update_music()
        {
            if (SDL_GetAudioStreamQueued(sound_system.music.stream) < (int)sound_system.music.wav_data_len) {
                /* feed more data to the stream. It will queue at the end, and trickle out as the hardware needs more data. */
                SDL_PutAudioStreamData(sound_system.music.stream, sound_system.music.wav_data, sound_system.music.wav_data_len);
            }
        }
    #pragma endregion Audio

    #pragma region Map
        void init_map()
        {
            for (int x = 0; x < deep::MAP_SIZE_X; ++x) { // Rows
                for (int y = 0; y < deep::MAP_SIZE_Y; ++y) { // Columns
                    map.map[x][y] = 0;
                }
            }
        }
        void add_mesh_to_map(int index, const char *filename, int rect)
        {
            if(index < map.meshes_max_count)
            {
                if(map.meshes[index].has_mesh)
                {
                    SDL_ReleaseGPUBuffer(render_context.device, map.meshes[index].vertex_buffer);
                    SDL_ReleaseGPUBuffer(render_context.device, map.meshes[index].index_buffer);
                }
                deep::Entity helper = {};
                load_gltf(filename, helper);
                map.meshes[index].has_mesh = true;
                map.meshes[index].vertex_buffer = helper.vertex_buffer;
                map.meshes[index].index_buffer = helper.index_buffer;                
                map.meshes[index].index_count = helper.index_count;

                map.meshes[index].is_collision_top = rect == 1 || rect == 2 || rect == 3;
                map.meshes[index].is_collision_right= rect == 3 || rect == 6 || rect == 9;
                map.meshes[index].is_collision_bottom = rect == 7 || rect == 8 || rect == 9;
                map.meshes[index].is_collision_left = rect == 1 || rect == 4 || rect == 7;
                map.meshes[index].has_any_collision = rect != 5;
            }
        }
        glm::vec3 map_position(int x, int y)
        {
            return glm::vec3(x*3.0f+1.5f, 0.0f, y*3.0f+1.5f);
        }
        void set_map(int x, int y, int tile)
        {
            if(x > -1 && y > -1 && x < deep::MAP_SIZE_X && y < deep::MAP_SIZE_Y && tile > -1 && tile < map.meshes_max_count)
            {
                map.map[y][x] = tile;
            }
        }

        Map_Mesh* get_map_mesh(int x , int z)
        {
            if (x < 0 || x >= deep::MAP_SIZE_X || z < 0 || z >= deep::MAP_SIZE_Y)
            {
                return nullptr;
            }
            return &map.meshes[map.map[z][x]-1];
        }
        bool is_position_blocked(glm::vec3 current_position, glm::vec3 next_position, float radius)
        {
            //return false;

            next_position = next_position/3.0f; // Grid is 3 Units
            radius = radius/3.0f;

            int min_gx = static_cast<int>(floor(next_position.x - radius));
            int max_gx = static_cast<int>(ceil(next_position.x + radius));
            int min_gz = static_cast<int>(floor(next_position.z - radius));
            int max_gz = static_cast<int>(ceil(next_position.z + radius));

            int current_x = static_cast<int>(floor(current_position.x/3.0f));
            int current_z = static_cast<int>(floor(current_position.z/3.0f));
            Map_Mesh* current_tile = get_map_mesh(current_x , current_z);
            if (current_tile == nullptr)
            {
                return false;
            }

            for (int gx = min_gx; gx <= max_gx; ++gx)
            {
                for (int gz = min_gz; gz <= max_gz; ++gz)
                {
                    if (gx >= 0 && gx < deep::MAP_SIZE_X && gz >= 0 && gz < deep::MAP_SIZE_Y)
                    {
                        float block_min_x = (float)gx;
                        float block_max_x = (float)gx + 1.0f;
                        float block_min_z = (float)gz;
                        float block_max_z = (float)gz + 1.0f;

                        float closest_x = glm::clamp(next_position.x, block_min_x, block_max_x);
                        float closest_z = glm::clamp(next_position.z, block_min_z, block_max_z);

                        float dist_x = next_position.x - closest_x;
                        float dist_z = next_position.z - closest_z;
                        float distance_squared = (dist_x * dist_x) + (dist_z * dist_z);

                        if (distance_squared < (radius * radius))
                        {
                            bool collision = false;
                            if(current_x+1 == gx && current_z == gz)
                            {
                                collision = current_tile->is_collision_right;
                            }
                            if(current_x-1 == gx && current_z == gz)
                            {
                                collision = current_tile->is_collision_left;
                            }
                            if(current_z+1 == gz && current_x == gx)
                            {
                                collision = current_tile->is_collision_bottom;
                            }
                            if(current_z-1 == gz && current_x == gx)
                            {
                                collision = current_tile->is_collision_top;
                            }

                            if(!current_tile->has_any_collision)
                            {
                                if (current_x + 1 == gx && current_z - 1 == gz)
                                {
                                    Map_Mesh* corner_tile = get_map_mesh(gx , gz);
                                    if (corner_tile != nullptr)
                                    {
                                        collision = corner_tile->is_collision_bottom || corner_tile->is_collision_left;
                                    }
                                }
                                else if (current_x - 1 == gx && current_z - 1 == gz)
                                {
                                    Map_Mesh* corner_tile = get_map_mesh(gx , gz);
                                    if (corner_tile != nullptr)
                                    {
                                        collision = corner_tile->is_collision_right || corner_tile->is_collision_bottom;
                                    }
                                }
                                else if (current_x + 1 == gx && current_z + 1 == gz)
                                {
                                    Map_Mesh* corner_tile = get_map_mesh(gx , gz);
                                    if (corner_tile != nullptr)
                                    {
                                        collision = corner_tile->is_collision_top || corner_tile->is_collision_left;
                                    }
                                }
                                else if (current_x - 1 == gx && current_z + 1 == gz)
                                {
                                    Map_Mesh* corner_tile = get_map_mesh(gx , gz);
                                    if (corner_tile != nullptr)
                                    {
                                        collision = corner_tile->is_collision_top || corner_tile->is_collision_right;
                                    }
                                }                                   
                            }

                            if(collision){
                                return collision;
                            }
                        }
                    }
                    else
                    {
                        return true; // Out of Bounce
                    }
                }
            }
            return false;
        }
    #pragma endregion Map

    #pragma region Game
        void init()
        {
            steam_init = SteamAPI_Init();
            if (steam_init) {
                SDL_Log("Steamworks API initialized successfully!");
                SDL_Log("Logged in as: %s", SteamFriends()->GetPersonaName());
            } else {
                SDL_Log("Failed to initialize Steamworks API.");
            }
            
            SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

            create_window();
            create_render_pipeline();
            create_depth_buffer();
            init_sound();
            setup_imgui();
            load_textures();
            camera_init(glm::vec3(0.0f, 0.0f, 0.0f));
            init_map();
        }
        void cleanup()
        {
            for (int i = 0; i < entities.count; ++i) {
                if(entities.data[i].mesh_component)
                {
                    SDL_ReleaseGPUBuffer(render_context.device, entities.data[i].vertex_buffer);
                    SDL_ReleaseGPUBuffer(render_context.device, entities.data[i].index_buffer);
                }
            }       
            for (int i = 0; i < map.meshes_count; ++i) {
                if(map.meshes[i].has_mesh)
                {
                    SDL_ReleaseGPUBuffer(render_context.device, map.meshes[i].vertex_buffer);
                    SDL_ReleaseGPUBuffer(render_context.device, map.meshes[i].index_buffer);
                }
            }         

            SDL_ReleaseGPUTexture(render_context.device, render_context.diffuse_map);
            SDL_ReleaseGPUTexture(render_context.device, render_context.specular_map);
            SDL_ReleaseGPUTexture(render_context.device, render_context.shininess_map);
            SDL_ReleaseGPUSampler(render_context.device, render_context.sampler);

            SDL_ReleaseGPUTexture(render_context.device, render_context.scene_depth_texture);

            SDL_ReleaseGPUGraphicsPipeline(render_context.device, render_context.graphics_pipeline); 

            ImGui_ImplSDL3_Shutdown();
            ImGui_ImplSDLGPU3_Shutdown();
            ImGui::DestroyContext();

            SDL_ReleaseWindowFromGPUDevice(render_context.device, render_context.window);
            SDL_DestroyGPUDevice(render_context.device);
            SDL_DestroyWindow(render_context.window);

            if (steam_init) {
                SteamAPI_Shutdown();
            }
        }

        double last_frame_time = 0;
        double get_delta_time() 
        { 
            double current_time = SDL_GetTicks() / 1000.0f;
            double delta_time = current_time - last_frame_time;
            last_frame_time = current_time;
            return delta_time;
        }

        void mouse_lock(bool lock) { SDL_SetWindowRelativeMouseMode(render_context.window, lock); }

        void clear_scene() {
            camera_init(glm::vec3(0.0f, 0.0f, 0.0f));

            for (int i = 0; i < entities.count; ++i) {
                if(entities.data[i].mesh_component)
                {
                    destroy_render_data(entities.data[i]);
                }
                entities.data[i].is_active = false;
                entities.data[i].transform = glm::mat4(1.0f);

                entities.data[i].light_component = false;
                entities.data[i].light_position = glm::vec3(0.0f, 0.0f, 0.0f);

                entities.data[i].mesh_component = false;

                entities.data[i].hurt_component = false;
            }
            entities.count = 0;
        }

        int create_entity()
        {
            if(entities.count < entities.max_count)
            {
                entities.data[entities.count].id = entities.count;
                entities.data[entities.count].is_active = true;
                entities.count += 1;
                return entities.count-1;
            }
            return -1;
        }

        int get_entity_count()
        {
            return entities.count;
        }

        deep::Entity* get_entity(int entity_id)
        {
            return &entities.data[entity_id];
        }

        void add_light(int entity_id, glm::vec3 position)
        {
            entities.data[entity_id].light_component = true;
            entities.data[entity_id].light_position = position;
        }

        void add_mesh(int entity_id, const char *filename, glm::vec3 position, glm::vec3 rotation)
        {
            deep::Entity &mesh = entities.data[entity_id];
            entities.data[entity_id].mesh_component = true;

            load_gltf(filename, mesh);
            
            mesh.transform = glm::mat4(1.0f);
            mesh.transform = glm::translate(mesh.transform, position);
            mesh.transform = glm::rotate(mesh.transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            mesh.transform = glm::rotate(mesh.transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            mesh.transform = glm::rotate(mesh.transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        } 
    #pragma endregion Game
    }

namespace deep
{
    #pragma region Interface
    void init() { deepcore::init(); }
    void cleanup(){ deepcore::cleanup(); }
    void update(){ deepcore::update_music(); deepcore::render(); }
    
    double get_delta_time() { return deepcore::get_delta_time(); }
    void mouse_lock(bool lock) { deepcore::mouse_lock(lock); }

    glm::vec2 get_camera_position_2d() { return deepcore::camera_get_position_2d(); }
    void set_camera_position(glm::vec3 position) { deepcore::camera_set_position(position); }
    void camera_process_keyboard(bool forward, bool back, bool left, bool right, bool up, bool down, float delta_time) { deepcore::camera_process_keyboard(forward, back, left, right, up, down, delta_time); }
    void camera_process_mouse_movement(float x_offset, float y_offset, bool constrain_pitch) { deepcore::camera_process_mouse_movement(x_offset, y_offset, constrain_pitch); }

    void clear_scene() { deepcore::clear_scene(); }
    int create_entity() { return deepcore::create_entity(); }
    int get_entity_count() { return deepcore::get_entity_count(); }
    Entity* get_entity(int entity_id) { return deepcore::get_entity(entity_id); }
    glm::vec2 get_entity_position_2d(Entity* entity) { glm::vec3 p = glm::vec3(entity->transform[3]); return glm::vec2(p.x, p.z); }
    void set_entity_position_2d(deep::Entity* entity, glm::vec2 new_entity_position)
    {
        glm::vec3 current_position_3d = glm::vec3(entity->transform[3]);
        glm::vec3 desired_target_position_3d = glm::vec3(new_entity_position.x, current_position_3d.y, new_entity_position.y);

        glm::vec3 desired_position_change = desired_target_position_3d - current_position_3d;

        glm::vec3 original_entity_position = current_position_3d;
        glm::vec3 temp_entity_position = current_position_3d;

        temp_entity_position.x = original_entity_position.x + desired_position_change.x;
        if (!deepcore::is_position_blocked(current_position_3d, temp_entity_position, entity->collision_radius))
        {
            entity->transform[3].x = temp_entity_position.x;
        }
        else
        {
            temp_entity_position.x = original_entity_position.x;
        }

        temp_entity_position.z = original_entity_position.z + desired_position_change.z;
        if (!deepcore::is_position_blocked(current_position_3d, glm::vec3(entity->transform[3].x, temp_entity_position.y, temp_entity_position.z), entity->collision_radius))
        {
            entity->transform[3].z = temp_entity_position.z;
        }
        else
        {
            temp_entity_position.z = original_entity_position.z;
        }
    }
    void add_light(int entity_id, glm::vec3 position) { deepcore::add_light(entity_id, position); }
    void add_mesh(int entity_id, const char *filename, glm::vec3 position, glm::vec3 rotation) { deepcore::add_mesh(entity_id, filename, position, rotation); }
    
    void load_music(const char *filename) { deepcore::load_music(filename); }
    int load_sound(const char *filename) { return deepcore::load_sound(filename); }
    void play_sound(int id) { deepcore::play_sound(id); }

    void init_map() { return deepcore::init_map(); }
    void add_mesh_to_map(int index, const char *filename, int rect) { return deepcore::add_mesh_to_map(index, filename, rect); }
    glm::vec3 map_position(int x, int y) { return deepcore::map_position(x, y); }
    void set_map(int x, int y, int tile) { return deepcore::set_map(x, y, tile); }
    #pragma endregion Interface
}