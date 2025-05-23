#include "game.h"

namespace deep
{
	Game::Game(const char* title)
	{	
		m_isGameRunning = m_renderer.init(title);
		m_renderer.addMesh();
	}

	void Game::run()
	{	
		while (m_isGameRunning)
		{
			update();
			draw();
		}
		m_renderer.quit();
	}

	void Game::update()
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

	void Game::draw()
	{
		m_renderer.render();
	}
}