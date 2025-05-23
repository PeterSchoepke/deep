#include "renderer.h"

namespace deep 
{
	bool Renderer::init(const char* title)
	{
		if (!SDL_Init(SDL_INIT_VIDEO))
		{
			std::cout << "SDL could not be initialized: " << SDL_GetError() << '\n';
			return false;
		}

		m_window = SDL_CreateWindow(title, 640, 480, SDL_WINDOW_RESIZABLE);
		if (m_window == nullptr)
		{
			std::cout << "SDL_Window was not able to be created\n";
			return false;
		}

		m_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
    	SDL_ClaimWindowForGPUDevice(m_device, m_window);

		createPipeline();

		return true;
	}

	void Renderer::render()
	{
		SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(m_device);

		SDL_GPUTexture* swapchainTexture;
		Uint32 width, height;
		SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, m_window, &swapchainTexture, &width, &height);

		if (swapchainTexture == NULL)
		{
			SDL_SubmitGPUCommandBuffer(commandBuffer);
		}
		else {
			SDL_GPUColorTargetInfo colorTargetInfo{};
			colorTargetInfo.clear_color = {0/255.0f, 91/255.0f, 150/255.0f, 255/255.0f};
			colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
			colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
			colorTargetInfo.texture = swapchainTexture;

			SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

			// draw something
				// bind the graphics pipeline
				SDL_BindGPUGraphicsPipeline(renderPass, m_graphicsPipeline);
				
				// bind the vertex buffer
				SDL_GPUBufferBinding bufferBindings[1];
				bufferBindings[0].buffer = m_meshes.at(0).getVertexBuffer(); // index 0 is slot 0 in this example
				bufferBindings[0].offset = 0; // start from the first byte

				SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings, 1); // bind one buffer starting from slot 0
				
				// issue a draw call
				SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

			SDL_EndGPURenderPass(renderPass);

			SDL_SubmitGPUCommandBuffer(commandBuffer);
		}
	}

	void Renderer::quit()
	{	
		for(Mesh& mesh : m_meshes)
		{
			mesh.destroy();
		}
		SDL_ReleaseGPUGraphicsPipeline(m_device, m_graphicsPipeline);
		SDL_DestroyGPUDevice(m_device);
		SDL_DestroyWindow(m_window);
		SDL_Quit();
	}

	void Renderer::addMesh()
	{
		m_meshes.emplace_back(m_device);
	}

	void Renderer::createPipeline()
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

		SDL_GPUShader* vertexShader = SDL_CreateGPUShader(m_device, &vertexInfo);

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

		SDL_GPUShader* fragmentShader = SDL_CreateGPUShader(m_device, &fragmentInfo);

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
		colorTargetDescriptions[0].format = SDL_GetGPUSwapchainTextureFormat(m_device, m_window);

		pipelineInfo.target_info.num_color_targets = 1;
		pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;

		// create the pipeline
		m_graphicsPipeline = SDL_CreateGPUGraphicsPipeline(m_device, &pipelineInfo);

		// we don't need to store the shaders after creating the pipeline
		SDL_ReleaseGPUShader(m_device, vertexShader);
		SDL_ReleaseGPUShader(m_device, fragmentShader);
	}
}