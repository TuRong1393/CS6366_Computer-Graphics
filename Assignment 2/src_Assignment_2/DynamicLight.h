#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>

class DynamicLight
{
public:
    glm::vec3 curCamPos; // current camera position
    glm::vec3 lightPos;  // light position

    DynamicLight(glm::vec3 initialPos)
    {
        // curCamPos = initialPos;
        // lightPos = initialPos;
        curCamPos = glm::vec3(initialPos.x, initialPos.y, initialPos.z);
        lightPos = glm::vec3(initialPos.x, initialPos.y, initialPos.z);
    }

    ~DynamicLight() {}

    /* set lightPos */
    void setLightPos(glm::vec3 pos)
    {
        // lightPos = pos;
        lightPos.x = pos.x;
        lightPos.y = pos.y;
        lightPos.z = pos.z;
    }

    /* Rotate light around x, y, z */
    void rotate(bool bRotateX, bool bRotateY, bool bRotateZ)
    {
        float deltaTime = glfwGetTime();
        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        if (bRotateX) // rotate around X
        {
            rotationMatrix = glm::rotate(rotationMatrix, deltaTime, glm::vec3(1, 0, 0));
        }
        if (bRotateY) // rotate around Y
        {
            rotationMatrix = glm::rotate(rotationMatrix, deltaTime, glm::vec3(0, 1, 0));
        }
        if (bRotateZ) // rotate around Z
        {
            rotationMatrix = glm::rotate(rotationMatrix, deltaTime, glm::vec3(0, 0, 1));
        }
        glm::vec4 res = rotationMatrix * glm::vec4(lightPos.x, lightPos.y, lightPos.z, 1);
        lightPos = glm::vec3(res.x, res.y, res.z);
    }

    /* Reset lightPos to curCamPos */
    void reset()
    {
        // lightPos = curCamPos;
        lightPos = glm::vec3(curCamPos.x, curCamPos.y, curCamPos.z);
    }
};