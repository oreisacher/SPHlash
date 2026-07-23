//
// Created by ruut on 3/14/26.
//

#include "../../../include/ui/panels/SimulationPanel.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "../../../include/rendering/Renderer.h"
#include "glm/gtc/type_ptr.hpp"

SimulationPanel::SimulationPanel(Editor* editor, Renderer* renderer)
    : UIPanel("Simulation"), editor(editor), renderer(renderer)
{ }

void SimulationPanel::preDraw(double deltaTime) {}

void SimulationPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());

    ImVec2 size = ImGui::GetContentRegionAvail();

    ImGui::Image(
        (void *)static_cast<intptr_t>(renderer->getTextureId()),
        ImVec2(viewportSize.x, viewportSize.y),
        ImVec2(0, 1),
        ImVec2(1, 0)
    );

    // Check if resized
    if (viewportSize.x != size.x || viewportSize.y != size.y) {
        viewportSize = { size.x, size.y };
        pendingResize = true;
        ImGui::End();
        return;
    }

    editor->getScene()->getCamera()->setHover(ImGui::IsWindowHovered());

    // --- Gizmo ---
    ImVec2 viewportPos = ImGui::GetItemRectMin();
    ImVec2 viewportSize = ImGui::GetItemRectSize();

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

    if (auto selected = editor->getSelected(); selected && selected->showGizmo()) {
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

void SimulationPanel::postDraw(double deltaTime) {
    if (pendingResize) {
        pendingResize = false;

        // Update systems
        ImVec2 size = { viewportSize.x, viewportSize.y };
        renderer->setSize(size.x, size.y);
        editor->updateRenderingSize(size.x, size.y);
        editor->getScene()->getCamera()->setAspectRatio(size.x / size.y);
    }
}
