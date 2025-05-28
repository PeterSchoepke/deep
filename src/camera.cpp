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

    void CameraProcessKeyboard(Camera& camera, bool forward, bool back, bool left, bool right, bool up, bool down, float deltaTime)
    {
        if(forward || back || left || right || up || down)
        {
            glm::vec3 movement = glm::vec3(0.0f, 0.0f, 0.0f);
            if(forward || back || left || right)
            {
                if(forward) { movement += camera.front; }
                if(back) { movement -= camera.front; }
                if(left) { movement -= camera.right; }
                if(right) { movement += camera.right; }
                movement.y = 0.0f;
                movement = glm::normalize(movement);
            }

            if(up) { movement.y += 1.0f; }
            if(down) { movement.y -= 1.0f; }

            float velocity = camera.movementSpeed * deltaTime;
            camera.position += movement * velocity;
        }
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