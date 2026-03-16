//
// Created by ruut on 3/14/26.
//

#include "../rendering/Framebuffer.h"
#include "UIManager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

UIManager::UIManager(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

UIManager::~UIManager() {
    for (auto& panel : panels)
        delete panel;
}

void UIManager::addPanel(UIPanel* panel) {
    panels.push_back(panel);
}

void UIManager::removePanel(UIPanel* panel) {
    std::erase(panels, panel);
}

void UIManager::removePanel(const std::string& name) {
    std::erase_if(panels, [name](const UIPanel* panel) { return panel->getName() == name; });
}

void UIManager::update(double deltaTime) const {
    for (auto& panel : panels)
        panel->update(deltaTime);
}

void UIManager::draw() const {
    Framebuffer::unbind();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

    for (auto& panel : panels)
        panel->draw();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
