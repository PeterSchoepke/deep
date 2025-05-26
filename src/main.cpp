#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "assets.h"
#include "render.h"
#include "camera.h"
#include "data.h"

deep::RenderContext renderContext{};
deep::Meshes meshes{};
deep::Camera camera{};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    deep::Create_Window(renderContext);
    deep::Create_Render_Pipeline(renderContext);
    deep::Create_Depth_Buffer(renderContext);
    deep::Load_Textures(renderContext);
    deep::InitCamera(camera);
    deep::Load_Meshes(renderContext, meshes);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    deep::Render(renderContext, camera, meshes);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    // close the window on request
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        return SDL_APP_SUCCESS;
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