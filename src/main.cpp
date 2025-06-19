#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include "engine.h"

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    deep::init();

    deep::camera_init(glm::vec3(0.0f, 0.0f, 3.0f));
    deep::mouse_lock(true);
    deep::load_meshes();
    deep::load_lights();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    double delta_time = deep::get_delta_time();

    const bool* KEYBOARD_STATE = SDL_GetKeyboardState(NULL);

    deep::camera_process_keyboard(
        KEYBOARD_STATE[SDL_SCANCODE_W],
        KEYBOARD_STATE[SDL_SCANCODE_S],
        KEYBOARD_STATE[SDL_SCANCODE_A],
        KEYBOARD_STATE[SDL_SCANCODE_D],
        KEYBOARD_STATE[SDL_SCANCODE_SPACE],
        KEYBOARD_STATE[SDL_SCANCODE_LSHIFT],
        delta_time
    );

    deep::render();
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
        float x_offset = static_cast<float>(event->motion.xrel);
        float y_offset = static_cast<float>(event->motion.yrel*-1);
        deep::camera_process_mouse_movement(x_offset, y_offset, true);
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    deep::cleanup();
}