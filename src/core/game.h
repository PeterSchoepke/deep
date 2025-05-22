#pragma once
#include "renderer.h"

namespace deep
{
	class Game
	{
	public:
		Game(const char* title);
		~Game();

		int run();

	private:
		const char* m_title = nullptr;
		Renderer m_renderer;
		bool m_isGameRunning{ true };

		void input();
		void preDraw();
		void draw();
	};
}