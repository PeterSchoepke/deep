#include "camera.h"

namespace deep
{
    void InitCamera(Camera& camera){
        camera.view = glm::mat4(1.0f);
        camera.view = glm::translate(camera.view, glm::vec3(0.0f, 0.0f, -3.0f)); 

        camera.projection = glm::perspective(glm::radians(45.0f), 640.0f / 480.0f, 0.1f, 100.0f);
    }
}