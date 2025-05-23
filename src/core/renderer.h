#pragma once
#include <iostream>
#include <vector>
#include <SDL3/SDL.h>
#include "mesh.h"

namespace deep 
{
	class Renderer 
	{
	public:
		bool init(const char* title);
		void render();
		void quit();

		void addMesh();

	private:
		SDL_Window* m_window = nullptr;
		SDL_GPUDevice* m_device = nullptr;
		SDL_GPUGraphicsPipeline* m_graphicsPipeline = nullptr;

		std::vector<Mesh> m_meshes;

		void createPipeline();
	}; 
}