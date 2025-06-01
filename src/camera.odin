package main

import "core:math"
import "core:math/linalg"
import sdl "vendor:sdl3"

camera_init :: proc(camera: ^Camera, position: Vec3) {
    camera.position = position;
        camera.world_up = { 0.0, 1.0, 0.0 };
        camera.yaw = -90.0;
        camera.pitch = 0.0;
        camera_update_vectors(camera);

        camera.movement_speed = 2.5;
        camera.mouse_sensitivity = 0.1;

        camera.projection = linalg.matrix4_perspective_f32(linalg.to_radians(f32(45)), 640.0 / 480.0, 0.1, 100.0);
}

camera_get_view_matrix :: proc(camera: ^Camera) -> Mat4 {
    return linalg.matrix4_look_at_f32(camera.position, camera.position + camera.front, camera.up)
}

camera_process_keyboard :: proc(camera: ^Camera, forward, back, left, right, up, down: bool, delta_time: f32) {
    if forward || back || left || right || up || down {
        movement: Vec3 = {0.0, 0.0, 0.0}
        if forward || back || left || right {
            if forward do movement += camera.front
            if back    do movement -= camera.front
            if left    do movement -= camera.right
            if right   do movement += camera.right
            movement.y = 0.0
            if linalg.length(movement) > 0.0 {
                movement = linalg.normalize(movement)
            }
        }

        if up   do movement.y += 1.0
        if down do movement.y -= 1.0

        velocity := camera.movement_speed * delta_time
        camera.position += movement * velocity
    }
}

camera_process_mouse_movement :: proc(camera: ^Camera, xoffset, yoffset: f32, constrain_pitch: bool) {
    xoffset := xoffset * camera.mouse_sensitivity
    yoffset := yoffset * camera.mouse_sensitivity

    camera.yaw += xoffset
    camera.pitch += yoffset

    if constrain_pitch {
        camera.pitch = math.clamp(camera.pitch, -89.0, 89.0)
    }

    camera_update_vectors(camera)
}

camera_update_vectors :: proc(camera: ^Camera) {
    front: Vec3 = { 0.0, 0.0, 0.0 };
    front.x = math.cos(linalg.to_radians(camera.yaw)) * math.cos(linalg.to_radians(camera.pitch));
    front.y = math.sin(linalg.to_radians(camera.pitch));
    front.z = math.sin(linalg.to_radians(camera.yaw)) * math.cos(linalg.to_radians(camera.pitch));
    camera.front = linalg.normalize(front);
    
    camera.right = linalg.normalize(linalg.cross(camera.front, camera.world_up));
    camera.up    = linalg.normalize(linalg.cross(camera.right, camera.front));
}