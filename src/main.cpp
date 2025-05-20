#include "core/game.h"

int main(int argc, char* argv[]) 
{	
	try
	{
		deep::Game dungeonCrawler("Crawler");
		return dungeonCrawler.run();
	}
	catch (const std::exception& e)
	{
		std::cin.get();
		return EXIT_FAILURE;
	}
	catch (...)
	{
		std::cin.get();
		return EXIT_FAILURE;
	}
}