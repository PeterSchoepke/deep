#include "render.h"

void DEEP_Create_Window(RenderContext& renderContext)
{
    // create a window
    renderContext.window = SDL_CreateWindow("Deep", 640, 480, SDL_WINDOW_RESIZABLE);
    
    // create the device
    renderContext.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    SDL_ClaimWindowForGPUDevice(renderContext.device, renderContext.window);
}
void DEEP_Destroy_Window(RenderContext& renderContext)
{
    // destroy the GPU device
    SDL_DestroyGPUDevice(renderContext.device);

    // destroy the window
    SDL_DestroyWindow(renderContext.window);
}

void DEEP_Create_Render_Pipeline(RenderContext& renderContext)
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
    vertexInfo.num_uniform_buffers = 0;

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
    fragmentInfo.num_samplers = 0;
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
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
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
void DEEP_Destroy_Render_Pipeline(RenderContext& renderContext)
{
    // release the pipeline
    SDL_ReleaseGPUGraphicsPipeline(renderContext.device, renderContext.graphicsPipeline);
}

void DEEP_Create_Render_Data(RenderContext& renderContext, RenderData& renderData)
{
    Vertex vertices[]
    {
        {0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},     // top vertex
        {-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f},   // bottom left vertex
        {0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}     // bottom right vertex
    };

    // create the vertex buffer
    SDL_GPUBufferCreateInfo bufferInfo{};
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    renderData.vertexBuffer = SDL_CreateGPUBuffer(renderContext.device, &bufferInfo);

    // create a transfer buffer to upload to the vertex buffer
    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.size = sizeof(vertices);
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    renderData.transferBuffer = SDL_CreateGPUTransferBuffer(renderContext.device, &transferInfo);

    // fill the transfer buffer
    Vertex* data = (Vertex*)SDL_MapGPUTransferBuffer(renderContext.device, renderData.transferBuffer, false);
    
    SDL_memcpy(data, (void*)vertices, sizeof(vertices));

    // data[0] = vertices[0];
    // data[1] = vertices[1];
    // data[2] = vertices[2];

    SDL_UnmapGPUTransferBuffer(renderContext.device, renderData.transferBuffer);

    // start a copy pass
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(renderContext.device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    // where is the data
    SDL_GPUTransferBufferLocation location{};
    location.transfer_buffer = renderData.transferBuffer;
    location.offset = 0;
    
    // where to upload the data
    SDL_GPUBufferRegion region{};
    region.buffer = renderData.vertexBuffer;
    region.size = sizeof(vertices);
    region.offset = 0;

    // upload the data
    SDL_UploadToGPUBuffer(copyPass, &location, &region, true);

    // end the copy pass
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
}
void DEEP_Destroy_Render_Data(RenderContext& renderContext, RenderData& renderData){
    // release buffers
    SDL_ReleaseGPUBuffer(renderContext.device, renderData.vertexBuffer);
    SDL_ReleaseGPUTransferBuffer(renderContext.device, renderData.transferBuffer);
}

void DEEP_Render(RenderContext& renderContext, RenderData& renderData)
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
    colorTargetInfo.clear_color = {0/255.0f, 91/255.0f, 150/255.0f, 255/255.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorTargetInfo.texture = swapchainTexture;

		// begin a render pass
		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

		// bind the pipeline
		SDL_BindGPUGraphicsPipeline(renderPass, renderContext.graphicsPipeline);

		// bind the vertex buffer
		SDL_GPUBufferBinding bufferBindings[1];
		bufferBindings[0].buffer = renderData.vertexBuffer;
		bufferBindings[0].offset = 0;

		SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings, 1);

		// issue a draw call
		SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

		// end the render pass
		SDL_EndGPURenderPass(renderPass);

    // submit the command buffer
    SDL_SubmitGPUCommandBuffer(commandBuffer);
}