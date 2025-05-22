#pragma once
#include <iostream>
#include <SDL3/SDL.h>

namespace deep 
{
	class Renderer 
	{
	public:
		Renderer();
		~Renderer();

		bool init(const char* title);
		void render();

	private:
		SDL_Window* m_window = nullptr;
		SDL_GPUDevice* m_device= nullptr;
	}; 
}