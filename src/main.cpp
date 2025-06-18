#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include "engine.h"

double lastFrameTime = 0;

deep::RenderContext renderContext{};
deep::Meshes meshes{};
deep::Lights lights{};
deep::Camera camera{};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    deep::Create_Window(renderContext);
    deep::Create_Render_Pipeline(renderContext);
    deep::Create_Depth_Buffer(renderContext);
    deep::Load_Textures(renderContext);
    deep::CameraInit(camera, glm::vec3(0.0f, 0.0f, 3.0f));
    deep::Load_Meshes(renderContext, meshes);
    deep::Load_Lights(lights);

    SDL_SetWindowRelativeMouseMode(renderContext.window, true);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    double currentTime = SDL_GetTicks() / 1000.0f;
    double deltaTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;

    const bool* keyboardState = SDL_GetKeyboardState(NULL);

    deep::CameraProcessKeyboard(
        camera, 
        keyboardState[SDL_SCANCODE_W],
        keyboardState[SDL_SCANCODE_S],
        keyboardState[SDL_SCANCODE_A],
        keyboardState[SDL_SCANCODE_D],
        keyboardState[SDL_SCANCODE_SPACE],
        keyboardState[SDL_SCANCODE_LSHIFT],
        deltaTime
    );

    deep::Render(renderContext, camera, meshes, lights);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_KEY_DOWN)
    {
        switch (event->key.key)
        {
            case SDLK_ESCAPE:
                return SDL_APP_SUCCESS;
            default:
                break;
        }
    }

    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        float xoffset = static_cast<float>(event->motion.xrel);
        float yoffset = static_cast<float>(event->motion.yrel*-1);
        deep::CameraProcessMouseMovement(camera, xoffset, yoffset, true);
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    deep::Destroy_Meshes(renderContext, meshes);
    deep::Destroy_Textures(renderContext);
    deep::Destroy_Depth_Buffer(renderContext);
    deep::Destroy_Render_Pipeline(renderContext);
    deep::Destroy_Window(renderContext);
}