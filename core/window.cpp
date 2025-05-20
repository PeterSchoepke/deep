#include "window.h"

namespace deep 
{
	Window::Window() 
	{
		
	}

	Window::~Window() 
	{
		SDL_DestroyWindow(m_window);
		SDL_Quit();
	}

	bool Window::init(const char* title)
	{
		bool success = true;

		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			std::cout << "SDL could not be initialized: " << SDL_GetError();
			success = false;
		}
		else
		{
			m_window = SDL_CreateWindow(
				title,
				50, // Placed the window at 50,50 because I was not able to move the window with 0,0
				50,
				640,
				480,
				SDL_WINDOW_SHOWN
			);
		}

		return success;
	}
}