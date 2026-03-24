//
// Created by ruut on 3/14/26.
//

#include "../rendering/Framebuffer.h"
#include "ui/UIManager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

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

    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

    // Set initial layout
    static bool layoutInitialized = false;
    if (!layoutInitialized) {
        layoutInitialized = true;

        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

        ImGuiID dock_main = dockspace_id;
        ImGuiID dock_right, dock_right_top, dock_right_bottom, dock_right_middle;

        dock_right  = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right,  0.40f, nullptr, &dock_main);
        dock_right_top = ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Up,  0.80f, nullptr, &dock_right_bottom);
        dock_right_top = ImGui::DockBuilderSplitNode(dock_right_top, ImGuiDir_Up,  0.50f, nullptr, &dock_right_middle);

        ImGui::DockBuilderDockWindow("Simulation", dock_main);
        ImGui::DockBuilderDockWindow("Settings", dock_right_top);
        ImGui::DockBuilderDockWindow("FPS", dock_right_bottom);
        ImGui::DockBuilderDockWindow("Particles", dock_right_middle);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    for (auto& panel : panels)
        panel->draw();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}