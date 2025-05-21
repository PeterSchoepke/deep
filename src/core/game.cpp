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
			while (m_isGameRunning)
			{
				input();
				preDraw();
				draw();
				m_window.swapWindow();
			}
		}

		return 0;
	}

	void Game::input()
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				m_isGameRunning = false;
			}
		}
	}

	void Game::preDraw()
	{
	}

	void Game::draw()
	{
	}
}