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

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Main colors - darker backgrounds, less saturated reds
    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.12f, 0.10f, 0.10f, 0.94f);  // Dark gray with hint of red
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.15f, 0.12f, 0.12f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.40f, 0.25f, 0.25f, 0.50f);  // Muted red border
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Frame colors - subtle red tint
    colors[ImGuiCol_FrameBg]                = ImVec4(0.18f, 0.14f, 0.14f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.28f, 0.18f, 0.18f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.35f, 0.20f, 0.20f, 0.67f);

    // Title bar
    colors[ImGuiCol_TitleBg]                = ImVec4(0.15f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.25f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.12f, 0.08f, 0.08f, 0.51f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.12f, 0.10f, 0.10f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.35f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.45f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.55f, 0.35f, 0.35f, 1.00f);

    // Checkboxes - brighter red accent
    colors[ImGuiCol_CheckMark]              = ImVec4(0.90f, 0.40f, 0.40f, 1.00f);

    // Tables
    colors[ImGuiCol_TableHeaderBg]        = ImVec4(0.22f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]    = ImVec4(0.45f, 0.28f, 0.28f, 0.80f);
    colors[ImGuiCol_TableBorderLight]     = ImVec4(0.30f, 0.20f, 0.20f, 0.50f);
    colors[ImGuiCol_TableRowBg]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]        = ImVec4(0.20f, 0.14f, 0.14f, 0.30f);

    // Sliders
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.60f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.80f, 0.45f, 0.45f, 1.00f);

    // Buttons - desaturated red
    colors[ImGuiCol_Button]                 = ImVec4(0.35f, 0.25f, 0.25f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.50f, 0.32f, 0.32f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.65f, 0.40f, 0.40f, 1.00f);

    // Headers
    colors[ImGuiCol_Header]                 = ImVec4(0.35f, 0.25f, 0.25f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.50f, 0.32f, 0.32f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.60f, 0.38f, 0.38f, 1.00f);

    // Separators
    colors[ImGuiCol_Separator]              = ImVec4(0.40f, 0.25f, 0.25f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.55f, 0.35f, 0.35f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.70f, 0.45f, 0.45f, 1.00f);

    // Resize grips
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.35f, 0.25f, 0.25f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.50f, 0.32f, 0.32f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.65f, 0.40f, 0.40f, 0.95f);

    // Tabs
    colors[ImGuiCol_Tab]                    = ImVec4(0.25f, 0.18f, 0.18f, 0.86f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.50f, 0.32f, 0.32f, 0.80f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.38f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.18f, 0.14f, 0.14f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.28f, 0.20f, 0.20f, 1.00f);

    // Text selection
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.50f, 0.32f, 0.32f, 0.35f);

    // Drag and drop
    colors[ImGuiCol_DragDropTarget]         = ImVec4(0.85f, 0.50f, 0.50f, 0.90f);

    // Navigation
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.80f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.15f, 0.10f, 0.10f, 0.35f);

    // Softer rounding
    style.WindowRounding    = 6.0f;
    style.FrameRounding     = 4.0f;
    style.GrabRounding      = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding       = 4.0f;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // ImGui::StyleColorsDark();

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
        ImGuiID dock_left, dock_left_1, dock_left_2, dock_left_3;
        ImGuiID dock_right, dock_right_1, dock_right_2, dock_right_3;

        ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.20f, &dock_left, &dock_main);
        ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.30f, &dock_right, &dock_main);

        // Left
        ImGui::DockBuilderSplitNode(dock_left, ImGuiDir_Down, 0.80f, &dock_left_2, &dock_left_1);
        ImGui::DockBuilderSplitNode(dock_left_2, ImGuiDir_Down, 0.30f, &dock_left_3, &dock_left_2);

        // Right
        ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down,  0.70f, &dock_right_2, &dock_right_1);
        ImGui::DockBuilderSplitNode(dock_right_2, ImGuiDir_Up,  0.50f, &dock_right_2, &dock_right_3);

        ImGui::DockBuilderDockWindow("Simulation", dock_main);
        ImGui::DockBuilderDockWindow("Simulation Settings", dock_right_1);
        ImGui::DockBuilderDockWindow("Rendering Settings", dock_right_2);
        ImGui::DockBuilderDockWindow("Particles", dock_right_3);
        ImGui::DockBuilderDockWindow("Controls", dock_left_1);
        ImGui::DockBuilderDockWindow("Init Settings", dock_left_2);
        ImGui::DockBuilderDockWindow("Scene", dock_left_2);
        ImGui::DockBuilderDockWindow("Statistics", dock_left_3);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    for (auto& panel : panels)
        panel->draw();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}