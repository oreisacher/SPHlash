//
// Created by ruut on 3/26/26.
//

#include "Camera.h"

#include "InputManager.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"


Camera::Camera(glm::vec3 position) : Position(position) {
    front = glm::vec3(0.0f, 0.0f, -1.0f);
    updateVectors();
}

void Camera::reset() {
    prevMousePosition = { -1, -1 };
}

void Camera::update(float deltaTime) {
    float velocity = MovementSpeed * deltaTime;

    // Movement
    if (Input.isKeyPressed(Window::Key::W))
        Position += front * velocity;
    if (Input.isKeyPressed(Window::Key::S))
        Position -= front * velocity;
    if (Input.isKeyPressed(Window::Key::A))
        Position -= right * velocity;
    if (Input.isKeyPressed(Window::Key::D))
        Position += right * velocity;

    // Mouse Look
    auto mousePos = Input.getMousePosition();

    float xDelta = prevMousePosition.x - mousePos.x;
    float yDelta = prevMousePosition.y - mousePos.y;

    if (prevMousePosition == glm::vec2(-1.0f, -1.0f)) {
        xDelta = 0;
        yDelta = 0;
    }

    prevMousePosition = mousePos;

    yaw -= xDelta * MouseSensitivity;
    pitch += yDelta * MouseSensitivity;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    updateVectors();
}

void Camera::setAspectRatio(const float aspect) {
    this->aspect = aspect;
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(Position, Position + front, up);
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(90.0f), aspect, 0.001f, 1000.0f);
}

void Camera::updateVectors() {
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);

    right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, front));
}
