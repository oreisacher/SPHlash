//
// Created by ruut on 3/16/26.
//

#include "rendering/Window.h"
#include <format>
#include <iostream>
#include "../utils/Logger.h"

GLFWwindow* Window::getNativeWindow() const { return window; }

Window::Window(const int width, const int height, const char *title)
    : width(width), height(height), title(title)
{
    if (!initialized) {
        LOG_ERROR("Window is not initialized. Call `Window::Init()` before use.")
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        return;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);

    // Cursor Callback
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) {
        auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win)
            return;

        for (auto &callback : win->mouseCallback)
            callback(win, x, y);
    });

    // Mouse Button Callback
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win)
            return;

        for (auto &callback : win->mouseButtonCallback)
            callback(win, static_cast<MouseButton>(button), static_cast<Action>(action), mods);
    });

    // Scroll Callback
    glfwSetScrollCallback(window, [](GLFWwindow* window, double x, double y) {
        auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win)
            return;

        for (auto &callback : win->mouseScrollCallback)
            callback(win, x, y);
    });

    // Key Callback
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win)
            return;

        for (auto &callback : win->keyCallback)
            callback(win, static_cast<Key>(key), static_cast<Action>(action), mods);
    });

    // Resize Callback
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        auto win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (!win)
            return;

        win->width = width;
        win->height = height;

        for (auto &callback : win->resizeCallback)
            callback(win, width, height);
    });
}

Window::~Window() {
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
}

void Window::Init() {
    glfwSetErrorCallback([](int error, const char* description) {
        std::cout << std::format("Error [{}]: {}", error, description) << std::endl;
    });

    // Initialize GLFW
    if (glfwInit()) {
        initialized = true;
    };
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(window);
}

void Window::close() const {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void Window::startFrame() const {
    glfwPollEvents();
}

void Window::endFrame() const {
    glfwSwapBuffers(window);
}

std::string Window::getTitle() const { return title; }

void Window::setTitle(std::string title) const {
    glfwSetWindowTitle(window, title.c_str());
}

void Window::setCurrent() const {
    glfwMakeContextCurrent(window);
}

void Window::setCursorMode(CursorMode mode) const {
    glfwSetInputMode(window, GLFW_CURSOR, static_cast<int>(mode));
}

void Window::addMouseCallback(const std::function<void(Window*, double, double)> &callback) {
    mouseCallback.push_back(callback);
}

void Window::addMouseButtonCallback(const std::function<void(Window*, MouseButton, Action, int)> &callback) {
    mouseButtonCallback.push_back(callback);
}

void Window::addMouseScrollCallback(const std::function<void(Window*, double, double)> &callback) {
    mouseScrollCallback.push_back(callback);
}

void Window::addKeyCallback(const std::function<void(Window*, Key, Action, int)> &callback) {
    keyCallback.push_back(callback);
}

void Window::addResizeCallback(const std::function<void(Window*, int, int)> &callback) {
    resizeCallback.push_back(callback);
}

float Window::getWidth() const { return width; }
float Window::getHeight() const { return height; }

float Window::getAspect() const {
    return static_cast<float>(width) / static_cast<float>(height);
}