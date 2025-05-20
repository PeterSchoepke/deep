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

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

		m_window = SDL_CreateWindow(
			title,
			50, // Placed the window at 50,50 because I was not able to move the window with 0,0
			50,
			640,
			480,
			SDL_WINDOW_OPENGL
		);
		if (m_window == nullptr)
		{
			std::cout << "SDL_Window was not able to be created\n";
			return false;
		}

		m_openGLContext = SDL_GL_CreateContext(m_window);
		if (m_openGLContext == nullptr)
		{
			std::cout << "OpenGL context not available\n";
			return false;
		}

		if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
		{
			std::cout << "glad was not initialized\n";
			return false;
		}

		std::cout << "Vendor: " << glGetString(GL_VENDOR) << '\n';
		std::cout << "Renderer: " << glGetString(GL_RENDERER) << '\n';
		std::cout << "Version: " << glGetString(GL_VERSION) << '\n';
		std::cout << "Shading Language: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';

		return true;
	}

	void Window::swapWindow()
	{
		SDL_GL_SwapWindow(m_window);
	}
}