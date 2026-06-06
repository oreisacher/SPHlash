//
// Created by ruut on 3/14/26.
//

#include "ui/SimulationPanel.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "../rendering/Renderer.h"
#include "glm/gtc/type_ptr.hpp"

SimulationPanel::SimulationPanel(Editor* editor, const Renderer* renderer)
    : UIPanel("Simulation"), editor(editor), texture(renderer->getTextureId())
{ }

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

    // --- Gizmo ---
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

    if (auto selected = editor->getSelected()) {
        static ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
        static ImGuizmo::MODE mode = ImGuizmo::MODE::WORLD;

        if (ImGui::IsKeyPressed(ImGuiKey_F))
            operation = ImGuizmo::TRANSLATE;

        if (ImGui::IsKeyPressed(ImGuiKey_G))
            operation = ImGuizmo::ROTATE;

        if (ImGui::IsKeyPressed(ImGuiKey_H))
            operation = ImGuizmo::SCALE;

        if (ImGui::IsKeyPressed(ImGuiKey_J))
            mode = ImGuizmo::MODE::WORLD;

        if (ImGui::IsKeyPressed(ImGuiKey_K))
            mode = ImGuizmo::MODE::LOCAL;

        ImGui::SetNextWindowPos({ viewportPos.x + 5, viewportPos.y + 5 });
        ImGui::BeginChild("##gizmoOverlay", ImVec2(0, 0));

        auto highlight = [](bool active) {
            return active ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f)   // yellow = active
                          : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);  // grey = inactive
        };

        ImGui::TextColored(highlight(operation == ImGuizmo::TRANSLATE), "[F] Translate");
        ImGui::SameLine();
        ImGui::TextColored(highlight(operation == ImGuizmo::ROTATE),    "[G] Rotate");
        ImGui::SameLine();
        ImGui::TextColored(highlight(operation == ImGuizmo::SCALE),     "[H] Scale");

        ImGui::Spacing();

        ImGui::TextColored(highlight(mode == ImGuizmo::WORLD), "[J] World");
        ImGui::SameLine();
        ImGui::TextColored(highlight(mode == ImGuizmo::LOCAL), "[K] Local");

        ImGui::EndChild();

        if (auto transform = dynamic_cast<Transform*>(selected)) {
            auto matrix = transform->getTransform();

            if (ImGuizmo::Manipulate(
                glm::value_ptr(editor->getScene()->getCamera()->getViewMatrix()),
                glm::value_ptr(editor->getScene()->getCamera()->getProjectionMatrix()),
                operation,
                mode,
                glm::value_ptr(matrix)
            )) {
                transform->setTransform(matrix);
            }
        }
    }

    ImGui::End();
}

std::vector<PanelEvent> SimulationPanel::getEvents() { return events; }