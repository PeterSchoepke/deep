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
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
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

		return true;
	}
}