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

    // // Debug print gradient lines
    // if (bField.resolution.x != 0) {
    //     std::vector<Hex32Cell> hex32cells(bField.resolution.x * bField.resolution.y * bField.resolution.z);
    //     cudaMemcpy(hex32cells.data(), bField.d_cells, bField.resolution.x * bField.resolution.y * bField.resolution.z * sizeof(Hex32Cell), cudaMemcpyDeviceToHost);
    //
    //     glm::mat4 mvp = camera->getProjectionMatrix() * camera->getViewMatrix();
    //
    //     ImDrawList* drawList = ImGui::GetForegroundDrawList();
    //
    //     int n = 0;
    //     for (auto cell : hex32cells) {
    //         if (cell.nodes[n] == 0) {
    //             continue;
    //         }
    //
    //         glm::vec4 position = { cell.worldPositions[n].x, cell.worldPositions[n].y, cell.worldPositions[n].z, 1.0f };
    //         glm::vec4 clip = mvp * position;
    //
    //         if (clip.w <= 0.0f) {
    //             continue;
    //         }
    //
    //         glm::vec3 ndc = glm::vec3(clip.x, clip.y, clip.z) / clip.w;
    //         ImVec2 screenPos = {
    //             viewportPos.x + (ndc.x * 0.5f + 0.5f) * viewportSize.x,
    //             viewportPos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * viewportSize.y
    //         };
    //
    //         glm::vec4 dirPos = { position.x + cell.nodeGradients[n].x * 0.1, position.y + cell.nodeGradients[n].y * 0.1, position.z + cell.nodeGradients[n].z * 0.1, 1.0f };
    //         clip = mvp * dirPos;
    //
    //         if (clip.w <= 0.0f) {
    //             continue;
    //         }
    //
    //         ndc = glm::vec3(clip.x, clip.y, clip.z) / clip.w;
    //         ImVec2 screenEndPos = {
    //             viewportPos.x + (ndc.x * 0.5f + 0.5f) * viewportSize.x,
    //             viewportPos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * viewportSize.y
    //         };
    //
    //         drawList->AddLine(
    //             screenPos,
    //             screenEndPos,
    //             IM_COL32(255, 0, 0, 255),
    //             2.0f
    //         );
    //     }
    // }

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