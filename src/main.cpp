#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "render.h"

RenderContext renderContext{};
RenderData renderData{};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    DEEP_Create_Window(renderContext);
    DEEP_Create_Render_Pipeline(renderContext);
    DEEP_Create_Render_Data(renderContext, renderData);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    DEEP_Render(renderContext, renderData);
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
    DEEP_Destroy_Render_Data(renderContext, renderData);
    DEEP_Destroy_Render_Pipeline(renderContext);
    DEEP_Destroy_Window(renderContext);
}