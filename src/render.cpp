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
            0, 1, 2,
            2, 3, 0,

            // Back face
            4, 5, 6,
            6, 7, 4,

            // Top face
            8, 9, 10,
            10, 11, 8,

            // Bottom face
            12, 13, 14,
            14, 15, 12,

            // Right face
            16, 17, 18,
            18, 19, 16,

            // Left face
            20, 21, 22,
            22, 23, 20
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

    void Render(RenderContext& renderContext, Camera& camera, Meshes& meshes)
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
            fragmentUniformBuffer.lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
            fragmentUniformBuffer.lightPosition = glm::vec3(1.2f, 1.0f, 2.0f);
            fragmentUniformBuffer.cameraPosition = camera.position;
            SDL_PushGPUFragmentUniformData(commandBuffer, 0, &fragmentUniformBuffer, sizeof(FragmentUniformBuffer));

            SDL_GPUTextureSamplerBinding textureSamplerBinding[2];
            textureSamplerBinding[0].texture = renderContext.texture;
            textureSamplerBinding[0].sampler = renderContext.sampler;
            textureSamplerBinding[1].texture = renderContext.sceneDepthTexture;
            textureSamplerBinding[1].sampler = renderContext.sampler;
            SDL_BindGPUFragmentSamplers(renderPass, 0, textureSamplerBinding, 1);
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
}