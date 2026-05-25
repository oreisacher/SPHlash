//
// Created by ruut on 3/14/26.
//

#include "ui/SimulationPanel.h"

#include <iostream>

#include "imgui.h"
#include "ImGuizmo.h"
#include "../../../../../../usr/lib/gcc/x86_64-pc-linux-gnu/14.3.1/include/c++/ostream"
#include "../rendering/FluidRenderer.h"
#include "utils/Hex32Field.h"

SimulationPanel::SimulationPanel(GLuint textureId) : UIPanel("Simulation"), texture(textureId) { }

void SimulationPanel::update(double deltaTime) {
    events.clear();
}

void SimulationPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());

    ImVec2 size = ImGui::GetContentRegionAvail();

    // Check if resized
    if (viewportSize.x != size.x || viewportSize.y != size.y) {
        viewportSize = { size.x, size.y };
        events.push_back({
            .type = PanelEvent::RESIZE,
            .resize = { viewportSize }
        });
    }

    ImGui::Image(
        (void *)(intptr_t)texture,
        size,
        ImVec2(0, 1),
        ImVec2(1, 0)
    );

    ImVec2 viewportPos = ImGui::GetItemRectMin();
    ImVec2 viewportSize = ImGui::GetItemRectSize();

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

    if (ImGui::IsItemHovered()) {
        // Mouse movement
        auto mouse = ImGui::GetMousePos();
        auto imageMin = ImGui::GetItemRectMin();

        glm::vec2 newMousePosition = { mouse.x - imageMin.x, mouse.y - imageMin.y };

        if (newMousePosition != mousePosition) {
            events.push_back({
                .type = PanelEvent::MOUSEMOVE,
                .mouseMove = { mousePosition, dragging, newMousePosition - mousePosition },
            });

            mousePosition = newMousePosition;
        }

        // Scroll
        if (ImGui::GetIO().MouseWheel != 0) {
            events.push_back({
                .type = PanelEvent::SCROLL,
                .scroll = { ImGui::GetIO().MouseWheel }
            });
        }

        // Buttons
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            dragging = true;
            events.push_back({
                .type = PanelEvent::MOUSEDOWN,
                .mouse = { 1 }
            });
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            dragging = true;
            events.push_back({
                .type = PanelEvent::MOUSEDOWN,
                .mouse = { 2 }
            });
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            dragging = false;
            events.push_back({
                .type = PanelEvent::MOUSEUP,
                .mouse = { 1 }
            });
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            dragging = false;
            events.push_back({
                .type = PanelEvent::MOUSEUP,
                .mouse = { 2 }
            });
        }
    }

    // Mouse Release outside if dragging
    if (dragging) {
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            dragging = false;
            events.push_back({
                .type = PanelEvent::MOUSEUP,
                .mouse = { 1 }
            });
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            dragging = false;
            events.push_back({
                .type = PanelEvent::MOUSEUP,
                .mouse = { 2 }
            });
        }
    }

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        events.push_back({
            .type = PanelEvent::MOUSEHOLD,
            .mouse = { 1 }
        });
    }

    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        events.push_back({
            .type = PanelEvent::MOUSEHOLD,
            .mouse = { 2 }
        });
    }

    ImGui::End();
}

std::vector<PanelEvent> SimulationPanel::getEvents() { return events; }