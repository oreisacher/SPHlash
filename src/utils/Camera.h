//
// Created by ruut on 3/26/26.
//

#ifndef FLUIDSIMSPH_CAMERA_H
#define FLUIDSIMSPH_CAMERA_H

#include "Config.h"
#include "glm/glm.hpp"

class Camera {
public:
    Camera(glm::vec3 position);

    void reset();
    void update(float deltaTime);

    void setAspectRatio(float aspect);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;

private:
    Config config;
    float movementSpeed = 2.5f;
    float mouseSensitivity = 0.1f;
    float keyboardSensitivity = 100.0f;
    float fov = 90.0f;

    void updateVectors();

    float yaw = -90.0f;
    float pitch = 0.0f;
    float aspect = 1.0f;

    const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::vec2 prevMousePosition = glm::vec2(-1.0f, -1.0f);
    glm::vec3 front, up, right;
};


#endif //FLUIDSIMSPH_CAMERA_H