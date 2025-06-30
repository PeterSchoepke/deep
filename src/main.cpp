#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include "engine.h"

const float PLAYER_ATTACK_DISTANCE = 4.0f;

enum UI_State 
{
    Running,
    Lose,
    Win
};

enum Audio 
{
    Attack,
    Hit,
    Hurt,
    Success
};

bool is_player_attacking = false;
int enemies_left = 0;
UI_State ui_state = UI_State::Running;

void load_scene()
{
    deep::set_camera_position(deep::map_position(10,10)+glm::vec3(0.0f, 1.8f, 0.0f));

    deep::add_light(deep::create_entity(), deep::map_position(18,18)+glm::vec3(0.0f, 1.5f, 0.0f));
    deep::add_light(deep::create_entity(), deep::map_position(18,1)+glm::vec3(0.0f, 1.5f, 0.0f));
    deep::add_light(deep::create_entity(), deep::map_position(1,18)+glm::vec3(0.0f, 1.5f, 0.0f));
    deep::add_light(deep::create_entity(), deep::map_position(1,1)+glm::vec3(0.0f, 1.5f, 0.0f));
    deep::add_mesh(deep::create_entity(), "ressources/models/floor.glb", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    deep::add_mesh(deep::create_entity(), "ressources/models/floor.glb", glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(180.0f, 0.0f, 0.0f));

    int enemy_id = deep::create_entity();
    deep::add_mesh(enemy_id, "ressources/models/cube.glb", deep::map_position(18,18)+glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    deep::get_entity(enemy_id)->hurt_component = true;
    enemy_id = deep::create_entity();
    deep::add_mesh(enemy_id, "ressources/models/cube.glb", deep::map_position(18,1)+glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    deep::get_entity(enemy_id)->hurt_component = true;
    enemy_id = deep::create_entity();
    deep::add_mesh(enemy_id, "ressources/models/cube.glb", deep::map_position(1,18)+glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    deep::get_entity(enemy_id)->hurt_component = true;
    enemy_id = deep::create_entity();
    deep::add_mesh(enemy_id, "ressources/models/cube.glb", deep::map_position(1,1)+glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    deep::get_entity(enemy_id)->hurt_component = true;
}

void update(float delta_time)
{
    if(ui_state == UI_State::Running)
    {
        glm::vec2 player_position = deep::get_camera_position_2d();
        int living_enemies = 0;
        for(int i =0; i < deep::get_entity_count(); i++)
        {
            deep::Entity* entity = deep::get_entity(i);
            if(entity->is_active && entity->hurt_component)
            {
                glm::vec2 entity_position = deep::get_entity_position_2d(entity);
                if(glm::distance(player_position, entity_position) < entity->sight)
                {
                    glm::vec2 direction = glm::normalize(player_position - entity_position);
                    glm::vec2 new_entity_position = entity_position + direction * entity->speed * delta_time;
                    deep::set_entity_position_2d(entity, new_entity_position);
                }
                if(glm::distance(player_position, entity_position) < entity->collision_radius)
                {
                    ui_state = UI_State::Lose;
                    deep::play_sound(Audio::Hurt);
                }
                if(is_player_attacking && glm::distance(player_position, entity_position) < PLAYER_ATTACK_DISTANCE)
                {
                    entity->is_active = false;
                    deep::play_sound(Audio::Hit);
                } else {
                    living_enemies++;
                }
            }
        }

        enemies_left = living_enemies;
        if(living_enemies == 0)
        {
            ui_state = UI_State::Win;
            deep::play_sound(Audio::Success);
        }

        is_player_attacking = false;
    }
}

void restart()
{
    deep::clear_scene();
    load_scene();
    ui_state = UI_State::Running;
}

void update_ui(float delta_time)
{
    switch (ui_state) {
        case Running:
            ImGui::Begin("HUD");
            //ImGui::Text("%.1f FPS", 1000.0f / (delta_time * 1000.0f)); 
            ImGui::Text("Enemies Left: %d", enemies_left);
            ImGui::End();
            break;
        case Win:
            ImGui::Begin("HUD");
            ImGui::Text("You Win");
            if (ImGui::Button("Restart"))
                restart();
            ImGui::End();
            break;
        case Lose:
            ImGui::Begin("HUD");
            ImGui::Text("Game Over");
            if (ImGui::Button("Restart"))
                restart();
            ImGui::End();
            break;
    }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    deep::init();

    deep::load_sound("attack.wav");
    deep::load_sound("hit.wav");
    deep::load_sound("hurt.wav");
    deep::load_sound("win.wav");
    deep::load_music("bg.wav");

    deep::add_mesh_to_map(0, "ressources/models/wall.glb");
    
    const int map[20][20] = {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,0,1,1,1,1,1,0,0,1,0,1,0,1,0,0,0,1},
        {1,0,1,0,1,0,0,0,1,0,0,1,0,1,0,1,0,0,0,1},
        {1,0,1,0,1,1,1,0,1,0,0,1,0,1,1,1,0,1,0,1},
        {1,0,1,0,1,0,0,0,0,0,0,1,0,1,0,0,0,1,0,1},
        {1,0,1,0,1,0,0,0,1,1,1,1,0,1,1,1,0,1,0,1},
        {1,0,1,0,1,0,0,0,0,0,0,1,0,1,0,0,0,1,0,1},
        {1,0,1,0,1,1,1,1,1,0,0,1,0,1,1,1,0,1,0,1},
        {1,0,1,0,1,0,0,0,0,0,0,1,0,1,0,0,0,1,0,1},
        {1,0,1,0,1,1,1,1,1,0,0,1,0,1,1,1,0,1,0,1},
        {1,0,1,0,1,0,0,0,0,0,0,1,0,1,0,0,0,1,0,1},
        {1,0,1,0,1,1,1,1,1,0,0,1,0,1,1,1,0,1,0,1},
        {1,0,1,0,1,0,0,0,0,0,0,1,0,1,0,0,0,1,0,1},
        {1,0,1,0,1,1,1,1,1,0,0,1,0,1,1,1,0,1,0,1},
        {1,0,1,0,1,0,0,0,0,0,0,1,0,1,0,0,0,1,0,1},
        {1,0,1,0,1,1,1,1,1,0,0,1,0,1,1,1,0,1,0,1},
        {1,0,1,0,1,0,0,0,0,0,0,1,0,1,0,0,0,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    };
    deep::set_map(map);

    load_scene();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    double delta_time = deep::get_delta_time();

    const bool* KEYBOARD_STATE = SDL_GetKeyboardState(NULL);

    if(ui_state == UI_State::Running)
    {
        deep::camera_process_keyboard(
            KEYBOARD_STATE[SDL_SCANCODE_W],
            KEYBOARD_STATE[SDL_SCANCODE_S],
            KEYBOARD_STATE[SDL_SCANCODE_A],
            KEYBOARD_STATE[SDL_SCANCODE_D],
            false, //KEYBOARD_STATE[SDL_SCANCODE_SPACE],
            false, //KEYBOARD_STATE[SDL_SCANCODE_LSHIFT],
            delta_time
        );
    }    

    deep::mouse_lock(ui_state == UI_State::Running);
    update(delta_time);
    update_ui(delta_time);
    deep::update();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    ImGui_ImplSDL3_ProcessEvent(event);

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
                if(ui_state == UI_State::Running)
                {
                    is_player_attacking = true;
                    deep::play_sound(Audio::Attack);
                }
                break;
            default:
                break;
        }
    }

    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        if(ui_state == UI_State::Running)
        {
            float x_offset = static_cast<float>(event->motion.xrel);
            float y_offset = static_cast<float>(event->motion.yrel*-1);
            deep::camera_process_mouse_movement(x_offset, y_offset, true);
        } 
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    deep::cleanup();
}