#include "renderer.h"

namespace deep 
{
	Renderer::Renderer() 
	{
		
	}

	Renderer::~Renderer() 
	{
		SDL_DestroyGPUDevice(m_device);
		SDL_DestroyWindow(m_window);
		SDL_Quit();
	}

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

			SDL_EndGPURenderPass(renderPass);

			SDL_SubmitGPUCommandBuffer(commandBuffer);
		}
	}
}