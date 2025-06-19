#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include "engine.h"

double last_frame_time = 0;

deep::Render_Context render_context{};
deep::Meshes meshes{};
deep::Lights lights{};
deep::Camera camera{};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    deep::init(render_context);

    deep::camera_init(camera, glm::vec3(0.0f, 0.0f, 3.0f));
    SDL_SetWindowRelativeMouseMode(render_context.window, true);
    deep::load_meshes(render_context, meshes);
    deep::load_lights(lights);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    double current_time = SDL_GetTicks() / 1000.0f;
    double delta_time = current_time - last_frame_time;
    last_frame_time = current_time;

    const bool* KEYBOARD_STATE = SDL_GetKeyboardState(NULL);

    deep::camera_process_keyboard(
        camera, 
        KEYBOARD_STATE[SDL_SCANCODE_W],
        KEYBOARD_STATE[SDL_SCANCODE_S],
        KEYBOARD_STATE[SDL_SCANCODE_A],
        KEYBOARD_STATE[SDL_SCANCODE_D],
        KEYBOARD_STATE[SDL_SCANCODE_SPACE],
        KEYBOARD_STATE[SDL_SCANCODE_LSHIFT],
        delta_time
    );

    deep::render(render_context, camera, meshes, lights);
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
        deep::camera_process_mouse_movement(camera, x_offset, y_offset, true);
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    deep::cleanup(render_context, meshes);
}