//
// Created by ruut on 10/24/26.
//

#include "ui/panels/RenderSettingsPanel.h"

#include "imgui.h"

RenderSettingsPanel::RenderSettingsPanel(Editor* editor, Renderer* renderer)
    : UIPanel("Render Settings"), editor(editor), renderer(renderer) { }

void RenderSettingsPanel::update(double deltaTime) { }

void RenderSettingsPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Automatic viewport resize:");
    ImGui::Checkbox("###automaticViewportResize", &renderer->settings.autoResizeViewport);

    ImGui::BeginDisabled(renderer->settings.autoResizeViewport);

    ImGui::Text("Width:");
    if (ImGui::DragInt("###vpWidth", &renderer->settings.viewportWidth, 1, 1, 8192, "%d", ImGuiSliderFlags_AlwaysClamp)) {
        editor->updateRenderingSize(
            renderer->settings.viewportScaling * renderer->settings.viewportWidth,
            renderer->settings.viewportScaling * renderer->settings.viewportHeight
        );
        renderer->updateSize();
    }

    ImGui::Text("Height:");
    if (ImGui::DragInt("###vpHeight", &renderer->settings.viewportHeight, 1, 1, 8192, "%d", ImGuiSliderFlags_AlwaysClamp)) {
        editor->updateRenderingSize(
            renderer->settings.viewportScaling * renderer->settings.viewportWidth,
            renderer->settings.viewportScaling * renderer->settings.viewportHeight
        );
        renderer->updateSize();
    }

    ImGui::EndDisabled();

    ImGui::Text("Scaling:");
    if (ImGui::DragFloat("###vpScaling", &renderer->settings.viewportScaling, 0.01f, 0.01f, 10.0f, "%f", ImGuiSliderFlags_AlwaysClamp)) {
        editor->updateRenderingSize(
            renderer->settings.viewportScaling * renderer->settings.viewportWidth,
            renderer->settings.viewportScaling * renderer->settings.viewportHeight
        );
        renderer->updateSize();
    }

    ImGui::End();
}
