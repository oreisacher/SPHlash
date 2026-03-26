//
// Created by ruut on 3/26/26.
//

#include "utils/InputManager.h"

void InputManager::addInputWindow(Window* window) {
    // Mouse Position
    window->addMouseCallback([&](Window *window, double x, double y) {
        mousePosition = { static_cast<int>(x), static_cast<int>(y) };
    });

    // Mouse Scroll Delta
    window->addMouseScrollCallback([&](Window *window, double x, double y) {
        mouseScrollDelta = { static_cast<int>(x), static_cast<int>(y) };
    });

    // Mouse Button
    window->addMouseButtonCallback([&](Window *window, Window::MouseButton button, Window::Action action, int mods) {
        mouseButtons[button] = action == Window::Action::PRESS;
    });

    // Key
    window->addKeyCallback([&](Window *window, Window::Key key, Window::Action action, int mods) {
        keys[key] = (action == Window::Action::PRESS || action == Window::Action::HOLD);
    });
}

void InputManager::reset() {
    mouseScrollDelta = glm::vec2(0.0f, 0.0f);
}

bool InputManager::isKeyPressed(Window::Key key) {
    auto it = keys.find(key);
    return it != keys.end() && it->second;
}

bool InputManager::isMouseButtonPressed(Window::MouseButton button) {
    auto it = mouseButtons.find(button);
    return it != mouseButtons.end() && it->second;
}

glm::vec2 InputManager::getMousePosition() const {
    return mousePosition;
}

glm::vec2 InputManager::getSrollDelta() const {
    return mouseScrollDelta;
}
