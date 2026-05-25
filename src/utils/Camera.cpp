//
// Created by ruut on 3/26/26.
//

#include "Camera.h"

#include "InputManager.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

Camera::Camera(glm::vec3 position) : position(position) {
    // Load config
    if (config.load("./configs/camera.json")) {
        movementSpeed           = config.get<float>("movementSpeed",        movementSpeed       );
        mouseSensitivity        = config.get<float>("mouseSensitivity",     mouseSensitivity    );
        keyboardSensitivity     = config.get<float>("keyboardSensitivity",  keyboardSensitivity );
        fov                     = config.get<float>("fov",                  fov                 );
    } else {
        config.set("movementSpeed",         movementSpeed       );
        config.set("mouseSensitivity",      mouseSensitivity    );
        config.set("keyboardSensitivity",   keyboardSensitivity );
        config.set("fov",                   fov                 );
        config.save();
    }

    front = glm::vec3(0.0f, 0.0f, -1.0f);
    updateVectors();
}

void Camera::reset() {
    prevMousePosition = { -1, -1 };
}

void Camera::update(float deltaTime) {
    float velocity = movementSpeed * deltaTime;

    // Movement
    if (Input.isKeyPressed(Window::Key::W))
        position += front * velocity;
    if (Input.isKeyPressed(Window::Key::S))
        position -= front * velocity;
    if (Input.isKeyPressed(Window::Key::A))
        position -= right * velocity;
    if (Input.isKeyPressed(Window::Key::D))
        position += right * velocity;

    if (Input.isKeyPressed(Window::Key::E))
        position += worldUp * velocity;
    if (Input.isKeyPressed(Window::Key::Q))
        position -= worldUp * velocity;

    // Mouse Look (MOUSE)
    if (Input.isMouseButtonPressed(Window::MouseButton::RIGHT)) {
        auto mousePos = Input.getMousePosition();

        float xDelta = prevMousePosition.x - mousePos.x;
        float yDelta = prevMousePosition.y - mousePos.y;

        if (prevMousePosition == glm::vec2(-1.0f, -1.0f)) {
            xDelta = 0;
            yDelta = 0;
        }

        prevMousePosition = mousePos;

        yaw -= xDelta * mouseSensitivity;
        pitch += yDelta * mouseSensitivity;
        pitch = glm::clamp(pitch, -89.0f, 89.0f);
    }

    // Mouse Look (Keyboard)
    if (Input.isKeyPressed(Window::Key::Up))
        pitch += keyboardSensitivity * deltaTime;
    if (Input.isKeyPressed(Window::Key::Down))
        pitch -= keyboardSensitivity * deltaTime;
    if (Input.isKeyPressed(Window::Key::Left))
        yaw -= keyboardSensitivity * deltaTime;
    if (Input.isKeyPressed(Window::Key::Right))
        yaw += keyboardSensitivity * deltaTime;

    updateVectors();
}

void Camera::setAspectRatio(const float aspect) {
    this->aspect = aspect;
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(fov), aspect, 0.001f, 1000.0f);
}

void Camera::updateVectors() {
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}
