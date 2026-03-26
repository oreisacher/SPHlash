//
// Created by ruut on 3/26/26.
//

#ifndef FLUIDSIMSPH_CAMERA_H
#define FLUIDSIMSPH_CAMERA_H

#include "glm/glm.hpp"

class Camera {
public:
    Camera(glm::vec3 position);

    void reset();
    void update(float deltaTime);

    void setAspectRatio(float aspect);

    float MovementSpeed = 2.5f;
    float MouseSensitivity = 0.1f;

    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;

private:
    void updateVectors();

    float yaw = -90.0f;
    float pitch = 0.0f;
    float aspect = 1.0f;

    glm::vec2 prevMousePosition = glm::vec2(-1.0f, -1.0f);
    glm::vec3 front, up, right;
};


#endif //FLUIDSIMSPH_CAMERA_H