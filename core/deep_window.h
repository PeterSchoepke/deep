#pragma once
#include <iostream>
#include <SDL2/SDL.h>
namespace deep 
{
	class Window 
	{
		public:
			Window(const char* title);
			~Window();

		private:
			SDL_Window* m_window = nullptr;
	}; 
}