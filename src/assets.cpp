#include "assets.h"

namespace deep
{
    SDL_Surface* Load_Image(const char* imageFilename, int desiredChannels)
    {
        char fullPath[256];
        SDL_Surface *result;
        SDL_PixelFormat format;

        SDL_snprintf(fullPath, sizeof(fullPath), "ressources/%s", imageFilename);

        result = SDL_LoadBMP(fullPath);
        if (result == NULL)
        {
            SDL_Log("Failed to load BMP: %s", SDL_GetError());
            return NULL;
        }

        if (desiredChannels == 4)
        {
            format = SDL_PIXELFORMAT_ABGR8888;
        }
        else
        {
            SDL_assert(!"Unexpected desiredChannels");
            SDL_DestroySurface(result);
            return NULL;
        }
        if (result->format != format)
        {
            SDL_Surface *next = SDL_ConvertSurface(result, format);
            SDL_DestroySurface(result);
            result = next;
        }

        return result;
    }
}