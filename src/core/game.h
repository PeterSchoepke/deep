#pragma once
#include <iostream>
#include "renderer.h"

namespace deep
{
	class Game
	{
	public:
		Game(const char* title);
		void run();

	private:
		Renderer m_renderer;
		bool m_isGameRunning{ false };

		void update();
		void draw();
	};
}