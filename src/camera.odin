package main

import "core:math"
import "core:math/linalg"
import sdl "vendor:sdl3"

cameraInit :: proc(camera: ^Camera, position: Vec3) {
    camera.position = position;
        camera.worldUp = { 0.0, 1.0, 0.0 };
        camera.yaw = -90.0;
        camera.pitch = 0.0;
        cameraUpdateVectors(camera);

        camera.movementSpeed = 2.5;
        camera.mouseSensitivity = 0.1;

        camera.projection = linalg.matrix4_perspective_f32(linalg.to_radians(f32(45)), 640.0 / 480.0, 0.1, 100.0);
}

cameraGetViewMatrix :: proc(camera: ^Camera) -> Mat4 {
    return linalg.matrix4_look_at_f32(camera.position, camera.position + camera.front, camera.up)
}

cameraProcessKeyboard :: proc(camera: ^Camera, forward, back, left, right, up, down: bool, deltaTime: f32) {
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

        velocity := camera.movementSpeed * deltaTime
        camera.position += movement * velocity
    }
}

cameraProcessMouseMovement :: proc(camera: ^Camera, xoffset, yoffset: f32, constrainPitch: bool) {
    xoffset := xoffset * camera.mouseSensitivity
    yoffset := yoffset * camera.mouseSensitivity

    camera.yaw += xoffset
    camera.pitch += yoffset

    if constrainPitch {
        camera.pitch = math.clamp(camera.pitch, -89.0, 89.0)
    }

    cameraUpdateVectors(camera)
}

cameraUpdateVectors :: proc(camera: ^Camera) {
    front: Vec3 = { 0.0, 0.0, 0.0 };
    front.x = math.cos(linalg.to_radians(camera.yaw)) * math.cos(linalg.to_radians(camera.pitch));
    front.y = math.sin(linalg.to_radians(camera.pitch));
    front.z = math.sin(linalg.to_radians(camera.yaw)) * math.cos(linalg.to_radians(camera.pitch));
    camera.front = linalg.normalize(front);
    
    camera.right = linalg.normalize(linalg.cross(camera.front, camera.worldUp));
    camera.up    = linalg.normalize(linalg.cross(camera.right, camera.front));
}