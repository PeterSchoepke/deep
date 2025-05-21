#pragma once
#include <iostream>
#include <SDL3/SDL.h>

namespace deep 
{
	class Window 
	{
	public:
		Window();
		~Window();

		bool init(const char* title);

	private:
		SDL_Window* m_window = nullptr;
	}; 
}