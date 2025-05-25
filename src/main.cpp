#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "render.h"

deep::RenderContext renderContext{};
deep::RenderData renderData{};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    deep::Create_Window(renderContext);
    deep::Create_Render_Pipeline(renderContext);
    deep::Create_Render_Data(renderContext, renderData);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    deep::Render(renderContext, renderData);
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
    deep::Destroy_Render_Data(renderContext, renderData);
    deep::Destroy_Render_Pipeline(renderContext);
    deep::Destroy_Window(renderContext);
}