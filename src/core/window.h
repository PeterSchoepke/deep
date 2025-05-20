#pragma once
#include <iostream>
#include <glad/glad.h>
#include <SDL2/SDL.h>

namespace deep 
{
	class Window 
	{
	public:
		Window();
		~Window();

		bool init(const char* title);
		void swapWindow();

	private:
		SDL_Window* m_window = nullptr;
		SDL_GLContext m_openGLContext = nullptr;
	}; 
}