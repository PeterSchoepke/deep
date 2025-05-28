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

    void Load_Meshes(RenderContext& renderContext, Meshes& meshes)
    {
        glm::vec3 cubePositions[] = {
            glm::vec3( 0.0f,  0.0f,  0.0f), 
            glm::vec3( 2.0f,  5.0f, -15.0f), 
            glm::vec3(-1.5f, -2.2f, -2.5f),  
            glm::vec3(-3.8f, -2.0f, -12.3f),  
            glm::vec3( 2.4f, -0.4f, -3.5f),  
            glm::vec3(-1.7f,  3.0f, -7.5f),  
            glm::vec3( 1.3f, -2.0f, -2.5f),  
            glm::vec3( 1.5f,  2.0f, -2.5f), 
            glm::vec3( 1.5f,  0.2f, -1.5f), 
            glm::vec3(1.2f, 1.0f, 2.0f)  
        };

        for(unsigned int i = 0; i < 10; i++)
        {
            Create_Render_Data(renderContext, meshes.data[i]);

            meshes.data[i].transform = glm::mat4(1.0f);
            meshes.data[i].transform = glm::translate(meshes.data[i].transform, cubePositions[i]);
            float angle = 20.0f * i; 
            meshes.data[i].transform = glm::rotate(meshes.data[i].transform, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
        }
        meshes.data[9].transform = glm::scale(meshes.data[9].transform, glm::vec3(0.1f));
        meshes.count = 10;
    }
    void Destroy_Meshes(RenderContext& renderContext, Meshes& meshes)
    {
        for (int i = 0; i < meshes.count; ++i) {
            Destroy_Render_Data(renderContext, meshes.data[i]);
        }
    }
}