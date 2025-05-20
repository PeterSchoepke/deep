#include "game.h"

namespace deep
{
	Game::Game(const char* title)
	{
		m_title = title;
	}

	Game::~Game()
	{
		
	}

	int Game::run()
	{
		if (m_window.init(m_title))
		{
			SDL_Event event;
			while (m_isGameRunning)
			{
				while (SDL_PollEvent(&event))
				{
					if (event.type == SDL_QUIT)
					{
						m_isGameRunning = false;
					}
				}
			}
		}

		return 0;
	}
}