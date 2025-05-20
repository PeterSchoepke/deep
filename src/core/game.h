#pragma once
#include "window.h"

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
		Window m_window;
		bool m_isGameRunning{ true };

		void input();
		void preDraw();
		void draw();
	};
}