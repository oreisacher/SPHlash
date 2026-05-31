//
// Created by ruut on 5/25/26.
//

#include "BoundariesPanel.h"

#include <iostream>

#include "imgui.h"
#include "Texture3DPanel.h"
#include "UIManager.h"
#include "glm/gtc/type_ptr.inl"

BoundariesPanel::BoundariesPanel(Editor* editor)
    : UIPanel("Boundaries"), editor(editor)
{
    boundaryBuilder = editor->getBoundaryBuilder();
}

void BoundariesPanel::update(double deltaTime) { }

void BoundariesPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    ImGui::Text("Resolution:");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Resolution of signed distance field and density map used by boundaries");
    ImGui::DragInt3("##bbres", glm::value_ptr(boundaryBuilder->settings.resolution), 1, 2, 1024, "%d", ImGuiSliderFlags_AlwaysClamp);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    Hex32Field boundaryField = editor->getBoundaryDensityMap();

    if (ImGui::Button(boundaryField.d_cells ? "Rebuild Boundary Map" : "Build Boundary Map", ImVec2(totalWidth, 0.0f))) {
        if (volumeMapPreview)
            volumeMapPreview->close();

        editor->buildBoundaryDensityMap();
    }

    if (boundaryField.d_cells) {
        if (ImGui::Button("Clear Boundary Map", ImVec2(totalWidth, 0.0f))) {
            if (volumeMapPreview)
                volumeMapPreview->close();

            editor->clearBoundaryDensityMap();
        }

        if (ImGui::Button(volumeMapPreview ? "Hide Boundary Map" : "Show Boundary Map", ImVec2(totalWidth, 0.0f))) {
            // Show density map preview
            if (volumeMapPreview == nullptr) {
                volumeMapPreview = new Texture3DPanel("Volume Map", boundaryField.texture, boundaryField.resolution.x, boundaryField.resolution.y, boundaryField.resolution.z);
                manager->addPanel(volumeMapPreview);

                volumeMapPreview->addOnCloseListener([this] () {
                    volumeMapPreview = nullptr;
                });
            } else {
                volumeMapPreview->close();
                volumeMapPreview = nullptr;
            }
        }
    }

    ImGui::End();
}