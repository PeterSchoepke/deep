#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "data.h"

namespace deep
{
    void CameraInit(Camera& camera, glm::vec3 position);
    glm::mat4 CameraGetViewMatrix(Camera& camera);
    void CameraProcessKeyboard(Camera& camera, SDL_Scancode key, float deltaTime);
    void CameraProcessMouseMovement(Camera& camera, float xoffset, float yoffset, bool constrainPitch = true);
    void CameraUpdateVectors(Camera& camera);
} 