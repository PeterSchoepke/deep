#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace deep
{
    struct Camera
    {
        glm::mat4 view;
        glm::mat4 projection;
    };

    void InitCamera(Camera& camera);
} 