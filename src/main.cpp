#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <random>
#include "engine.h"

const float PLAYER_ATTACK_DISTANCE = 4.0f;

static SDL_Joystick* joystick = nullptr;
SDL_JoystickID joystick_id = 0;

std::mt19937 rng(std::random_device{}());
int randi_range(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

struct Room {
    static const int SIZE_X = 5;
    static const int SIZE_Y = 3;

    int data[SIZE_Y][SIZE_X];

    Room()
    {
        for (int y = 0; y < SIZE_Y; ++y) 
        {
            for (int x = 0; x < SIZE_X; ++x) 
            {
                if (y == 0) 
                {
                    data[y][x] = 2;
                } 
                else if (x == SIZE_X - 1) 
                {
                    data[y][x] = 6;
                } 
                else if (y == SIZE_Y - 1) 
                {
                    data[y][x] = 8;
                } 
                else if (x == 0) 
                {
                    data[y][x] = 4;
                } 
                else 
                {
                    data[y][x] = 5;
                }
            }
        }

        data[0][0] = 1;
        data[0][SIZE_X - 1] = 3;
        data[SIZE_Y - 1][0] = 7;
        data[SIZE_Y - 1][SIZE_X - 1] = 9;
    }    
};
struct Procedural_Map {
    static const int SIZE_X = 7;
    static const int SIZE_Y = 5;

    int data[SIZE_Y][SIZE_X];
    glm::bvec4 doors[SIZE_Y][SIZE_X];
    bool recalculate_doors[SIZE_Y][SIZE_X];

    Procedural_Map()
    {
        for (int y = 0; y < SIZE_Y; ++y) 
        {
            for (int x = 0; x < SIZE_X; ++x)
            {
                data[y][x] = 0;
                doors[y][x] = glm::bvec4(false, false, false, false);
                recalculate_doors[y][x] = false;
            }
        }
    }
};

void add_doors(glm::ivec2 pos, glm::bvec4 doors)
{
    if(doors.x) // top
    {
        int map_x = pos.x + 2;
        int map_y = pos.y;
        if (map_x >= 0 && map_x < deep::MAP_SIZE_X &&
            map_y >= 0 && map_y < deep::MAP_SIZE_Y) 
        {
            deep::set_map(map_x, map_y, 10);
        }
    }
    if(doors.y) // right
    {
        int map_x = pos.x + Room::SIZE_X - 1;
        int map_y = pos.y + 1;
        if (map_x >= 0 && map_x < deep::MAP_SIZE_X &&
            map_y >= 0 && map_y < deep::MAP_SIZE_Y) 
        {
            deep::set_map(map_x, map_y, 12);
        }
    }
    if(doors.z) // bottom
    {
        int map_x = pos.x + 2;
        int map_y = pos.y + Room::SIZE_Y - 1;
        if (map_x >= 0 && map_x < deep::MAP_SIZE_X &&
            map_y >= 0 && map_y < deep::MAP_SIZE_Y) 
        {
            deep::set_map(map_x, map_y, 13);
        }
    }
    if(doors.w) // left
    {
        int map_x = pos.x;
        int map_y = pos.y + 1;
        if (map_x >= 0 && map_x < deep::MAP_SIZE_X &&
            map_y >= 0 && map_y < deep::MAP_SIZE_Y) 
        {
            deep::set_map(map_x, map_y, 11);
        }
    }
}

void add_room(Room& room, glm::ivec2 pos, glm::bvec4 doors)
{
    for (int y = 0; y < Room::SIZE_Y; ++y) 
    {
        for (int x = 0; x < Room::SIZE_X; ++x) 
        {
            int map_x = pos.x + x;
            int map_y = pos.y + y;

            if (map_x >= 0 && map_x < deep::MAP_SIZE_X &&
                map_y >= 0 && map_y < deep::MAP_SIZE_Y) {
                deep::set_map(map_x, map_y, room.data[y][x]);
            }
        }
    }
    add_doors(pos, doors);
}

void add_rooms(Procedural_Map generative_map, Room& room)
{
    for (int y = 0; y < Procedural_Map::SIZE_Y; ++y) 
    {
        for (int x = 0; x < Procedural_Map::SIZE_X; ++x) 
        {
            int current = generative_map.data[y][x];
            if(current > 0)
            {                
                add_room(room, glm::ivec2(x*Room::SIZE_X, y*Room::SIZE_Y), generative_map.doors[y][x]);
            }
        }
    }
}

void procgen_place_entrance(Procedural_Map& map, glm::ivec2& start_position)
{
    start_position.x = randi_range(0, Procedural_Map::SIZE_X-1);
    start_position.y = randi_range(0, Procedural_Map::SIZE_Y-1);
    map.data[start_position.y][start_position.x] = 1;
    map.recalculate_doors[start_position.y][start_position.x] = true;
}

void procgen_calculate_doors(Procedural_Map& map)
{
    for (int y = 0; y < Procedural_Map::SIZE_Y; ++y) 
    {
        for (int x = 0; x < Procedural_Map::SIZE_X; ++x) 
        {
            if(map.recalculate_doors[y][x])
            {
                int current = map.data[y][x];
                if(current > 0)
                {
                    glm::bvec4 doors_for_this_room = map.doors[y][x];
                    if (y > 0) 
                    {
                        int top = map.data[y - 1][x];
                        bool recalculate = map.recalculate_doors[y - 1][x];
                        if(recalculate && top != 0 && (current-1 == top || current+1 == top))
                        {
                            doors_for_this_room.x = true;
                        }
                    }
                    if (x < Procedural_Map::SIZE_X - 1) 
                    {
                        int right = map.data[y][x + 1];
                        bool recalculate = map.recalculate_doors[y][x + 1];
                        if(recalculate && right != 0 && (current-1 == right || current+1 == right))
                        {
                            doors_for_this_room.y = true;
                        }       
                    }
                    if (y < Procedural_Map::SIZE_Y - 1) 
                    {
                        int bottom = map.data[y + 1][x];
                        bool recalculate = map.recalculate_doors[y + 1][x];
                        if(recalculate && bottom != 0 && (current-1 == bottom || current+1 == bottom))
                        {
                            doors_for_this_room.z = true;
                        }
                    }
                    if (x > 0) 
                    {
                        int left = map.data[y][x - 1];
                        bool recalculate = map.recalculate_doors[y][x - 1];
                        if(recalculate && left != 0 && (current-1 == left || current+1 == left))
                        {
                            doors_for_this_room.w = true;
                        }
                    }

                    map.doors[y][x] = doors_for_this_room;
                }
            }
        }
    }

    for (int y = 0; y < Procedural_Map::SIZE_Y; ++y) 
    {
        for (int x = 0; x < Procedural_Map::SIZE_X; ++x) 
        {
            
            map.recalculate_doors[y][x] = false;
        }
    }
}

bool procgen_generate_path(Procedural_Map& map, glm::ivec2 from, int length, const int marker, std::vector<glm::ivec2>& branch_candidates) {
        if (length == 0) {
            procgen_calculate_doors(map);
            return true;
        }

        map.recalculate_doors[from.y][from.x] = true;
        glm::ivec2 current = from;
        glm::ivec2 direction;

        // Representing directions: UP, RIGHT, DOWN, LEFT
        std::vector<glm::ivec2> directions = {
            glm::ivec2(0, 1),  // UP
            glm::ivec2(1, 0),  // RIGHT
            glm::ivec2(0, -1), // DOWN
            glm::ivec2(-1, 0)  // LEFT
        };

        // Shuffle directions to randomize path generation
        std::shuffle(directions.begin(), directions.end(), rng);

        for (int i = 0; i < 4; ++i) {
            direction = directions[i];
            glm::ivec2 next_pos = current + direction;

            if (next_pos.x >= 0 && next_pos.x < Procedural_Map::SIZE_X &&
                next_pos.y >= 0 && next_pos.y < Procedural_Map::SIZE_Y &&
                map.data[next_pos.y][next_pos.x] == 0) { // Check if unoccupied
                
                current = next_pos;
                map.data[current.y][current.x] = marker;
                map.recalculate_doors[current.y][current.x] = true;

                if (length > 1) {
                    branch_candidates.push_back(current);
                }

                if (procgen_generate_path(map, current, length - 1, marker+1, branch_candidates)) {
                    return true;
                } else {
                    // Backtrack: Remove from candidates and clear dungeon cell
                    auto it = std::find(branch_candidates.begin(), branch_candidates.end(), current);
                    if (it != branch_candidates.end()) {
                        branch_candidates.erase(it);
                    }
                    map.data[current.y][current.x] = 0; // Set back to unoccupied
                    map.recalculate_doors[current.y][current.x] = false;
                    current = from; // Revert current to 'from' for the next iteration of the loop
                }
            }
        }
        return false;
    }

void procgen_generate_branches(Procedural_Map& map, std::vector<glm::ivec2>& branch_candidates) 
{
    int branches_created = 0;
    glm::ivec2 candidate;
    while (branches_created < 3 && !branch_candidates.empty()) {
        int random_index = randi_range(0, branch_candidates.size() - 1);
        candidate = branch_candidates[random_index];

        if (procgen_generate_path(map, candidate, randi_range(1, 4), map.data[candidate.y][candidate.x]+1, branch_candidates)) {
            branches_created++;
        } else {
            // Remove the candidate if it didn't lead to a successful branch
            auto it = std::find(branch_candidates.begin(), branch_candidates.end(), candidate);
            if (it != branch_candidates.end()) {
                branch_candidates.erase(it);
            }
        }
    }
}

void procgen_find_exit(Procedural_Map& map, glm::ivec2& exit_position)
{
    int max_room_number = 0;
    exit_position = glm::ivec2(0, 0);
    for (int y = 0; y < Procedural_Map::SIZE_Y; ++y) 
    {
        for (int x = 0; x < Procedural_Map::SIZE_X; ++x) 
        {
            if(map.data[y][x] > max_room_number)
            {
                max_room_number = map.data[y][x];
                exit_position.x = x;
                exit_position.y = y;
            }
        }
    }
}

glm::vec3 position_inside_room(glm::ivec2& room_position, int x, int y)
{
    return glm::vec3((room_position.x*Room::SIZE_X*3)+x*3.0f+1.5f, 0.0f, (room_position.y*Room::SIZE_Y*3)+y*3.0f+1.5f);
}

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

struct Player {
    bool is_player_attacking = false;
};
Player players[2];
int player_count = 1;

int enemies_left = 0;
UI_State ui_state = UI_State::Running;

void load_scene()
{
    Room room = {};
    Procedural_Map map = {};
    
    glm::ivec2 start_position;
    procgen_place_entrance(map, start_position);

    std::vector<glm::ivec2> branch_candidates;    
    procgen_generate_path(map, start_position, 13, 2, branch_candidates);
    glm::ivec2 goal_position;
    procgen_find_exit(map, goal_position);
    procgen_generate_branches(map, branch_candidates);

    add_rooms(map, room);

    glm::vec3 spawn_position = position_inside_room(start_position, 1, 1);
    deep::set_camera_position(0, spawn_position+glm::vec3(0.0f, 1.8f, 0.0f));
    deep::add_mesh(deep::create_entity(), "ressources/models/player.glb", spawn_position, glm::vec3(0.0f, 0.0f, 0.0f));
    if(player_count > 1)
    {
        deep::set_camera_position(1, position_inside_room(start_position, 1, 1)+glm::vec3(0.0f, 1.8f, 0.0f));
        deep::add_mesh(deep::create_entity(), "ressources/models/player.glb", spawn_position, glm::vec3(0.0f, 0.0f, 0.0f));
    }

    int exit_id = deep::create_entity();
    deep::add_mesh(exit_id, "ressources/models/center.glb", position_inside_room(goal_position, 2, 1), glm::vec3(0.0f, 0.0f, 0.0f));
    deep::get_entity(exit_id)->exit_component = true;

    if (!branch_candidates.empty())
    {
        int parts = branch_candidates.size() / 5;

        deep::add_light(deep::create_entity(), position_inside_room(branch_candidates[parts*1-1], 1, 1)+glm::vec3(0.0f, 2.5f, 0.0f));
        deep::add_light(deep::create_entity(), position_inside_room(branch_candidates[parts*2-1], 1, 1)+glm::vec3(0.0f, 2.5f, 0.0f));
        deep::add_light(deep::create_entity(), position_inside_room(branch_candidates[parts*3-1], 1, 1)+glm::vec3(0.0f, 2.5f, 0.0f));
        deep::add_light(deep::create_entity(), position_inside_room(branch_candidates[parts*4-1], 1, 1)+glm::vec3(0.0f, 2.5f, 0.0f));

        int enemy_id = deep::create_entity();
        deep::add_mesh(enemy_id, "ressources/models/cube.glb", position_inside_room(branch_candidates[parts*1-1], 1, 1)+glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        deep::get_entity(enemy_id)->hurt_component = true;
        enemy_id = deep::create_entity();
        deep::add_mesh(enemy_id, "ressources/models/cube.glb", position_inside_room(branch_candidates[parts*2-1], 1, 1)+glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        deep::get_entity(enemy_id)->hurt_component = true;
        enemy_id = deep::create_entity();
        deep::add_mesh(enemy_id, "ressources/models/cube.glb", position_inside_room(branch_candidates[parts*3-1], 1, 1)+glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        deep::get_entity(enemy_id)->hurt_component = true;
        enemy_id = deep::create_entity();
        deep::add_mesh(enemy_id, "ressources/models/cube.glb", position_inside_room(branch_candidates[parts*4-1], 1, 1)+glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        deep::get_entity(enemy_id)->hurt_component = true;
    }    
}

void update(float delta_time)
{
    if(ui_state == UI_State::Running)
    {        
        int living_enemies = 0;
        for(int i = 0; i < deep::get_entity_count(); i++)
        {
            deep::Entity* entity = deep::get_entity(i);

            int nearest_player = 0;
            float nearest_player_distance = 9999;
            for(int player_id = 0; player_id < player_count; player_id++)
            {
                glm::vec2 player_position = deep::get_camera_position_2d(player_id);

                if(entity->is_active)
                {
                    if(entity->hurt_component)
                    {
                        glm::vec2 entity_position = deep::get_entity_position_2d(entity);

                        float player_distance = glm::distance(player_position, entity_position);
                        if(player_distance < nearest_player_distance)
                        {
                            nearest_player = player_id;
                            nearest_player_distance = player_distance;
                        }

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
                        if(players[player_id].is_player_attacking)
                        {
                            if(players[player_id].is_player_attacking && glm::distance(player_position, entity_position) < PLAYER_ATTACK_DISTANCE)
                            {
                                entity->is_active = false;
                                deep::play_sound(Audio::Hit);
                            }
                        }
                    }

                    if(entity->exit_component)
                    {
                        glm::vec2 entity_position = deep::get_entity_position_2d(entity);
                        if(glm::distance(player_position, entity_position) < entity->collision_radius)
                        {
                            ui_state = UI_State::Win;
                            deep::play_sound(Audio::Success);
                        }
                    }
                }
                
            }
            if(nearest_player_distance < entity->sight)
            {
                glm::vec2 player_position = deep::get_camera_position_2d(nearest_player);
                glm::vec2 entity_position = deep::get_entity_position_2d(entity);
                glm::vec2 direction = glm::normalize(player_position - entity_position);
                glm::vec2 new_entity_position = entity_position + direction * entity->speed * delta_time;
                deep::set_entity_position_2d(entity, new_entity_position);
            }

            if(entity->is_active && entity->hurt_component)
            {
                living_enemies++;
            }
        }

        for(int player_id = 0; player_id < player_count; player_id++)
        {
            glm::vec2 player_position = deep::get_camera_position_2d(player_id);
            deep::Entity* entity = deep::get_entity(player_id);
            deep::set_entity_position_2d(entity, player_position);
            players[player_id].is_player_attacking = false;
        }
        
        enemies_left = living_enemies;
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
            ImGui::Text("Player 1 Position: X%.1f Y%.1f Z%.1f", deepcore::cameras[0].position.x, deepcore::cameras[0].position.y, deepcore::cameras[0].position.z);
            //ImGui::Text("Player 1 Tile: X%d Z%d", static_cast<int>(floor(deepcore::cameras[0].position.x/3.0f)), static_cast<int>(floor(deepcore::cameras[0].position.z/3.0f)));
            ImGui::Text("Player 2 Position: X%.1f Y%.1f Z%.1f", deepcore::cameras[1].position.x, deepcore::cameras[1].position.y, deepcore::cameras[1].position.z);
            //ImGui::Text("Player 2 Tile: X%d Z%d", static_cast<int>(floor(deepcore::cameras[1].position.x/3.0f)), static_cast<int>(floor(deepcore::cameras[1].position.z/3.0f)));
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
    deep::use_both_monitors = true;
    if(deep::use_both_monitors)
    {
        player_count = 2;
    }
    deep::init();

    deep::load_sound("attack.wav");
    deep::load_sound("hit.wav");
    deep::load_sound("hurt.wav");
    deep::load_sound("win.wav");
    deep::load_music("bg.wav");

    deep::add_mesh_to_map(0, "ressources/models/wall_1.glb", 1);
    deep::add_mesh_to_map(1, "ressources/models/wall_2.glb", 2);
    deep::add_mesh_to_map(2, "ressources/models/wall_3.glb", 3);
    deep::add_mesh_to_map(3, "ressources/models/wall_4.glb", 4);
    deep::add_mesh_to_map(4, "ressources/models/wall_5.glb", 5);
    deep::add_mesh_to_map(5, "ressources/models/wall_6.glb", 6);
    deep::add_mesh_to_map(6, "ressources/models/wall_7.glb", 7);
    deep::add_mesh_to_map(7, "ressources/models/wall_8.glb", 8);
    deep::add_mesh_to_map(8, "ressources/models/wall_9.glb", 9);
    deep::add_mesh_to_map(9, "ressources/models/wall_2_door.glb", 5);
    deep::add_mesh_to_map(10, "ressources/models/wall_4_door.glb", 5);
    deep::add_mesh_to_map(11, "ressources/models/wall_6_door.glb", 5);
    deep::add_mesh_to_map(12, "ressources/models/wall_8_door.glb", 5);

    load_scene();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    double delta_time = deep::get_delta_time();

    if(ui_state == UI_State::Running)
    {
        const bool* KEYBOARD_STATE = SDL_GetKeyboardState(NULL);
        bool forward = KEYBOARD_STATE[SDL_SCANCODE_W];
        bool back = KEYBOARD_STATE[SDL_SCANCODE_S];
        bool left = KEYBOARD_STATE[SDL_SCANCODE_A];
        bool right = KEYBOARD_STATE[SDL_SCANCODE_D];

        deep::camera_process_keyboard(
            0,
            forward,
            back,
            left,
            right,
            false, //KEYBOARD_STATE[SDL_SCANCODE_SPACE],
            false, //KEYBOARD_STATE[SDL_SCANCODE_LSHIFT],
            delta_time
        );

        if (joystick)
        {
            Uint8 hat_state = SDL_GetJoystickHat(joystick, 0);
            forward = hat_state & SDL_HAT_UP;
            back = hat_state & SDL_HAT_DOWN;
            left = hat_state & SDL_HAT_LEFT;
            right = hat_state & SDL_HAT_RIGHT;

            const int JOYSTICK_DEAD_ZONE = 8000;
            Sint16 x_axis = SDL_GetJoystickAxis(joystick, 0);
            Sint16 y_axis = SDL_GetJoystickAxis(joystick, 1);
            forward = forward || y_axis < -JOYSTICK_DEAD_ZONE;
            back = back || y_axis > JOYSTICK_DEAD_ZONE;
            left = left || x_axis < -JOYSTICK_DEAD_ZONE;
            right = right || x_axis > JOYSTICK_DEAD_ZONE;
        }

        deep::camera_process_keyboard(
            1,
            forward,
            back,
            left,
            right,
            false, //KEYBOARD_STATE[SDL_SCANCODE_SPACE],
            false, //KEYBOARD_STATE[SDL_SCANCODE_LSHIFT],
            delta_time
        );

        Sint16 axis_right_x_value = SDL_GetJoystickAxis(joystick, 2);
        Sint16 axis_right_y_value = SDL_GetJoystickAxis(joystick, 3);
        const Sint16 JOYSTICK_DEAD_ZONE = 8000;
        float x_offset = 0.0f;
        float y_offset = 0.0f;
        if (axis_right_x_value > JOYSTICK_DEAD_ZONE || axis_right_x_value < -JOYSTICK_DEAD_ZONE)
        {
            x_offset = static_cast<float>(axis_right_x_value) / SDL_JOYSTICK_AXIS_MAX;
        }
        if (axis_right_y_value > JOYSTICK_DEAD_ZONE || axis_right_y_value < -JOYSTICK_DEAD_ZONE)
        {
            y_offset = static_cast<float>(axis_right_y_value) / SDL_JOYSTICK_AXIS_MAX * -1.0f;
        }
        const float CAMERA_SENSITIVITY = 1000.0f * delta_time;
        deep::camera_process_mouse_movement(1, x_offset * CAMERA_SENSITIVITY, y_offset * CAMERA_SENSITIVITY, true);
    }

    deep::mouse_lock(ui_state == UI_State::Running);
    update(delta_time);
    update_ui(delta_time);
    deep::update();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if(ui_state != UI_State::Running)
    {
        ImGui_ImplSDL3_ProcessEvent(event);
    }

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
                    players[0].is_player_attacking = true;
                    deep::play_sound(Audio::Attack);
                }
                break;
            default:
                break;
        }
    }

    if (event->type == SDL_EVENT_JOYSTICK_BUTTON_DOWN)
    {
        if (ui_state == UI_State::Running && joystick && event->jbutton.which == joystick_id)
        {
            switch (event->jbutton.button)
            {
                case 10:
                    players[1].is_player_attacking = true;
                    deep::play_sound(Audio::Attack);
                    break;
                default:
                    break;
            }
        }
    }

    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        if(ui_state == UI_State::Running)
        {
            float x_offset = static_cast<float>(event->motion.xrel);
            float y_offset = static_cast<float>(event->motion.yrel*-1);
            deep::camera_process_mouse_movement(0, x_offset, y_offset, true);
        } 
    }    

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    } else if (event->type == SDL_EVENT_JOYSTICK_ADDED) {
        /* this event is sent for each hotplugged stick, but also each already-connected joystick during SDL_Init(). */
        if (joystick == NULL) {  /* we don't have a stick yet and one was added, open it! */
            joystick = SDL_OpenJoystick(event->jdevice.which);
            if (!joystick) 
            {
                SDL_Log("Failed to open joystick ID %u: %s", (unsigned int) event->jdevice.which, SDL_GetError());
            } 
            else
            {
                joystick_id = event->jdevice.which;
                SDL_Log("Success to open joystick ID %u: %s", (unsigned int) event->jdevice.which, SDL_GetJoystickName(joystick));
            }
        }
    } else if (event->type == SDL_EVENT_JOYSTICK_REMOVED) {
        if (joystick && (SDL_GetJoystickID(joystick) == event->jdevice.which)) {
            SDL_CloseJoystick(joystick);  /* our joystick was unplugged. */
            joystick = NULL;
            joystick_id = 0;
        }
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    deep::cleanup();
}