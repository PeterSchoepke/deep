#include "deep_window.h"

namespace deep 
{
	Window::Window(const char* title) 
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0) 
		{
			std::cout << "SDL could not be initialized: " << SDL_GetError();
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

			bool isGameRunning = true;
			SDL_Event event;
			while (isGameRunning)
			{
				while (SDL_PollEvent(&event)) 
				{
					if (event.type == SDL_QUIT) 
					{
						isGameRunning = false;
					}
				}
			}

			SDL_DestroyWindow(m_window);
		}

		SDL_Quit();
	}

	Window::~Window() 
	{
		SDL_DestroyWindow(m_window);
	}
}