#include "camera.h"

namespace deep
{
    void CameraInit(Camera& camera, glm::vec3 position)
    {
        camera.position = position;
        camera.worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        camera.yaw = -90.0f;
        camera.pitch = 0.0f;
        CameraUpdateVectors(camera);

        camera.movementSpeed = 2.5f;
        camera.mouseSensitivity = 0.1f;

        camera.projection = glm::perspective(glm::radians(45.0f), 640.0f / 480.0f, 0.1f, 100.0f);
    }

    glm::mat4 CameraGetViewMatrix(Camera& camera)
    {
        return glm::lookAt(camera.position, camera.position + camera.front, camera.up);
    }

    void CameraProcessKeyboard(Camera& camera, SDL_Scancode key, float deltaTime)
    {
        float velocity = camera.movementSpeed * deltaTime;
        if (key == SDL_SCANCODE_W)
            camera.position += camera.front * velocity;
        if (key == SDL_SCANCODE_S)
            camera.position -= camera.front * velocity;
        if (key == SDL_SCANCODE_A)
            camera.position -= camera.right * velocity;
        if (key == SDL_SCANCODE_D)
            camera.position += camera.right * velocity;
        if (key == SDL_SCANCODE_SPACE)
            camera.position += glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
        if (key == SDL_SCANCODE_LSHIFT)
            camera.position += glm::vec3(0.0f, -1.0f, 0.0f) * velocity;
    }

    void CameraProcessMouseMovement(Camera& camera, float xoffset, float yoffset, bool constrainPitch)
    {
        xoffset *= camera.mouseSensitivity;
        yoffset *= camera.mouseSensitivity;

        camera.yaw   += xoffset;
        camera.pitch += yoffset;

        if (constrainPitch)
        {
            if (camera.pitch > 89.0f)
                camera.pitch = 89.0f;
            if (camera.pitch < -89.0f)
                camera.pitch = -89.0f;
        }

        CameraUpdateVectors(camera);
    }

    void CameraUpdateVectors(Camera& camera)
    {
        glm::vec3 front;
        front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        front.y = sin(glm::radians(camera.pitch));
        front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        camera.front = glm::normalize(front);
        
        camera.right = glm::normalize(glm::cross(camera.front, camera.worldUp));
        camera.up    = glm::normalize(glm::cross(camera.right, camera.front));
    }
}