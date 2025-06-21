#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <glm/glm/glm.hpp>
#include "engine.h"
#include <iostream>

bool is_player_attacking = false;

bool update(float delta_time)
{
    glm::vec2 player_position = deep::get_camera_position_2d();
    int living_enemies = 0;
    for(int i =0; i < deep::get_entity_count(); i++)
    {
        deep::Entity* entity = deep::get_entity(i);
        if(entity->is_active && entity->hurt_component)
        {
            glm::vec2 entity_position = deep::get_entity_position_2d(entity);
            if(glm::distance(player_position, entity_position) < 14.0f)
            {
                glm::vec2 direction = glm::normalize(player_position - entity_position);
                glm::vec2 new_entity_position = entity_position + direction * 3.0f * delta_time;
                deep::set_entity_position_2d(entity, new_entity_position);
            }
            if(glm::distance(player_position, entity_position) < 0.5f)
            {
                SDL_Log("Game Over");
                return true;
            }
            if(is_player_attacking && glm::distance(player_position, entity_position) < 1.75f)
            {
                entity->is_active = false;
            } else {
                living_enemies++;
            }
        }
    }

    if(living_enemies == 0)
    {
        SDL_Log("You Win");
        return true;
    }

    is_player_attacking = false;
    return false;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    deep::init();

    deep::set_camera_position(glm::vec3(0.0f, 1.8f, 0.0f));
    deep::mouse_lock(true);

    deep::add_light(deep::create_entity(), glm::vec3(10.0f, 4.0f, 10.0f));
    deep::add_light(deep::create_entity(), glm::vec3(-10.0f, 4.0f, 10.0f));
    deep::add_light(deep::create_entity(), glm::vec3(10.0f, 4.0f, -10.0f));
    deep::add_light(deep::create_entity(), glm::vec3(-10.0f, 4.0f, -10.0f));
    deep::add_mesh(deep::create_entity(), "ressources/models/floor.glb", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    int enemy_id = deep::create_entity();
    deep::add_mesh(enemy_id, "ressources/models/cube.glb", glm::vec3(10.0f, 0.5f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    deep::get_entity(enemy_id)->hurt_component = true;
    enemy_id = deep::create_entity();
    deep::add_mesh(enemy_id, "ressources/models/cube.glb", glm::vec3(-10.0f, 0.5f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    deep::get_entity(enemy_id)->hurt_component = true;
    enemy_id = deep::create_entity();
    deep::add_mesh(enemy_id, "ressources/models/cube.glb", glm::vec3(10.0f, 0.5f, -10.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    deep::get_entity(enemy_id)->hurt_component = true;
    enemy_id = deep::create_entity();
    deep::add_mesh(enemy_id, "ressources/models/cube.glb", glm::vec3(-10.0f, 0.5f, -10.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    deep::get_entity(enemy_id)->hurt_component = true;

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

    if(update(delta_time)) { return SDL_APP_SUCCESS; }

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

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        switch (event->button.button)
        {
            case SDL_BUTTON_LEFT:
                is_player_attacking = true;
                break;
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