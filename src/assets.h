#pragma once

#include <SDL3/SDL.h>

namespace deep
{
    SDL_Surface* Load_Image(const char* imageFilename, int desiredChannels);
} 