#pragma once
#include <SDL3/SDL.h>

namespace deep 
{
	struct Vertex
	{
		float x, y, z;      //vec3 position
		float r, g, b, a;   //vec4 color
	};

	class Mesh 
	{
	public:
		Mesh(SDL_GPUDevice* device);
		void destroy();

		SDL_GPUBuffer* getVertexBuffer();
	
	private:
		Vertex vertices[21] = {
			{0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},     // top vertex
			{-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f},   // bottom left vertex
			{0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}     // bottom right vertex
		};

		SDL_GPUDevice* m_device = nullptr;
		SDL_GPUBuffer* m_vertexBuffer = nullptr;
		SDL_GPUTransferBuffer* m_transferBuffer = nullptr;
	}; 
}