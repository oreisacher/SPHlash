//
// Created by ruut on 3/14/26.
//

#include "ui/panels/SimulationPanel.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "rendering/Renderer.h"
#include "glm/gtc/type_ptr.hpp"
#include <iostream>

SimulationPanel::SimulationPanel(Editor* editor, Renderer* renderer)
    : UIPanel("Simulation"), editor(editor), renderer(renderer) {
}

void SimulationPanel::update(double deltaTime) {}

void SimulationPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());

    auto framebuffer = renderer->getFramebuffer();
    glm::vec2 fbSize = framebuffer->getSize();

    ImVec2 size = ImGui::GetContentRegionAvail();

    // Check if resized
    if (renderer->settings.autoResizeViewport && (
        renderer->settings.viewportWidth != size.x ||
        renderer->settings.viewportHeight != size.y)
    ) {
        renderer->settings.viewportWidth = size.x;
        renderer->settings.viewportHeight = size.y;
        editor->updateRenderingSize(
            renderer->settings.viewportScaling * size.x,
            renderer->settings.viewportScaling * size.y
        );
        renderer->updateSize();
        ImGui::End();
        return;
    }

    float textureAspect = fbSize.x / fbSize.y;
    float windowAspect = size.x / size.y;

    ImVec2 imageSize;

    if (windowAspect > textureAspect) {
        imageSize.x = size.y * textureAspect;
        imageSize.y = size.y;
    } else {
        imageSize.x = size.x;
        imageSize.y = size.x / textureAspect;
    }

    float offsetX = (size.x - imageSize.x) * 0.5f;
    float offsetY = (size.y - imageSize.y) * 0.5f;

    ImGui::SetCursorPos(ImVec2(
            ImGui::GetCursorPosX() + offsetX,
            ImGui::GetCursorPosY() + offsetY
        )
    );

    ImGui::Image(
        (void *)static_cast<intptr_t>(renderer->getTextureId()),
        imageSize,
        ImVec2(0, 1),
        ImVec2(1, 0)
    );

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