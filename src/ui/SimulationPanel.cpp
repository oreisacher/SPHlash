//
// Created by ruut on 3/14/26.
//

#include "ui/SimulationPanel.h"
#include "imgui.h"
#include "../rendering/FluidRenderer.h"

SimulationPanel::SimulationPanel(const std::string& name, GLuint texture) : UIPanel(name), texture(texture) { }

void SimulationPanel::update(double deltaTime) { }

void SimulationPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());

    ImVec2 size = ImGui::GetContentRegionAvail();

    if (viewportSize.x != size.x || viewportSize.y != size.y) {
        viewportSize = { size.x, size.y };
        resized = true;
    }

    ImGui::Image(
        (void *)(intptr_t)texture,
        size,
        ImVec2(0, 1),
        ImVec2(1, 0)
    );

    if (ImGui::IsItemHovered()) {
        auto mouse = ImGui::GetMousePos();
        auto imageMin = ImGui::GetItemRectMin();
        mousePosition = { mouse.x - imageMin.x, mouse.y - imageMin.y };
        mouseScroll = ImGui::GetIO().MouseWheel;
    } else {
        mousePosition = { -100, -100 };
    }

    ImGui::End();
}

glm::vec2 SimulationPanel::getSize() const { return viewportSize; }
bool SimulationPanel::wasResized() const { return resized; }
void SimulationPanel::clearResizeFlag() { resized = false; }

int SimulationPanel::isMouseDown() const {
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        return 1;
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
        return 2;
    return 0;
}

glm::vec2 SimulationPanel::getMousePosition() const {
    return mousePosition;
}

float SimulationPanel::getScroll() const {
    return mouseScroll;
}
