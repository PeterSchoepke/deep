#include "render.h"

namespace deep
{
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
        fragmentInfo.num_samplers = 1;
        fragmentInfo.num_storage_buffers = 0;
        fragmentInfo.num_storage_textures = 0;
        fragmentInfo.num_uniform_buffers = 0;

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
        SDL_GPUVertexAttribute vertexAttributes[2];

        // a_position
        vertexAttributes[0].buffer_slot = 0;
        vertexAttributes[0].location = 0;
        vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertexAttributes[0].offset = 0;

        // a_color
        vertexAttributes[1].buffer_slot = 0;
        vertexAttributes[1].location = 1;
        vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertexAttributes[1].offset = sizeof(float) * 3;

        pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
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

    void Load_Textures(RenderContext& renderContext)
    {
        SDL_Surface *imageData = Load_Image("diffuse.bmp", 4);
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
        renderContext.texture = SDL_CreateGPUTexture(renderContext.device, &textureCreateInfo);

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
        textureRegion.texture = renderContext.texture;
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
    }
    void Destroy_Textures(RenderContext& renderContext)
    {
        // release the texture and sampler
        SDL_ReleaseGPUTexture(renderContext.device, renderContext.texture);
        SDL_ReleaseGPUSampler(renderContext.device, renderContext.sampler);
    }

    void Create_Render_Data(RenderContext& renderContext, RenderData& renderData)
    {
        Vertex vertices[]
        {
            {-0.5f,  0.5f, 0.0f,   0.0f, 1.0f}, // top left
            { 0.5f,  0.5f, 0.0f,   1.0f, 1.0f}, // top right
            { 0.5f, -0.5f, 0.0f,   1.0f, 0.0f}, // bottom right
            {-0.5f, -0.5f, 0.0f,   0.0f, 0.0f}  // bottom left
        };
        Uint16 indices[] = {  
            0, 1, 2, // first triangle
            0, 2, 3  // second triangle
        };

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
        Uint16* indexData = (Uint16*) &transferData[4];
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

    void Render(RenderContext& renderContext, Camera& camera, RenderData& renderData)
    {
        VertexUniformBuffer vertexUniformBuffer{};
        vertexUniformBuffer.model = renderData.transform;
        vertexUniformBuffer.view = camera.view;
        vertexUniformBuffer.projection = camera.projection;

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
        colorTargetInfo.clear_color = {0/255.0f, 91/255.0f, 150/255.0f, 255/255.0f};
        colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
        colorTargetInfo.texture = swapchainTexture;

            SDL_PushGPUFragmentUniformData(commandBuffer, 0, &vertexUniformBuffer, sizeof(VertexUniformBuffer));

            // begin a render pass
            SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

            // bind the pipeline
            SDL_BindGPUGraphicsPipeline(renderPass, renderContext.graphicsPipeline);

            // bind the vertex buffer
            SDL_GPUBufferBinding vertexBufferBinding{};
            vertexBufferBinding.buffer = renderData.vertexBuffer;
            vertexBufferBinding.offset = 0;
            SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

            SDL_GPUBufferBinding indexBufferBinding{};
            indexBufferBinding.buffer = renderData.indexBuffer;
            indexBufferBinding.offset = 0;
            SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

            SDL_GPUTextureSamplerBinding textureSamplerBinding{};
            textureSamplerBinding.texture = renderContext.texture;
            textureSamplerBinding.sampler = renderContext.sampler;
            SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSamplerBinding, 1);

            // issue a draw call
            SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);

            // end the render pass
            SDL_EndGPURenderPass(renderPass);

        // submit the command buffer
        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }
}