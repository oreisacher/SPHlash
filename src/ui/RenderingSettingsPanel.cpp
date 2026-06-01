//
// Created by ruut on 4/12/26.
//

#include "RenderingSettingsPanel.h"

#include <iostream>

#include "imgui.h"
#include "glm/gtc/type_ptr.hpp"

RenderingSettingsPanel::RenderingSettingsPanel(FluidRenderer* renderer, FluidSurfaceMesher* surfaceMesher)
    : UIPanel("Rendering Settings"), renderer(renderer), surfaceMesher(surfaceMesher) {
}

void RenderingSettingsPanel::update(double deltaTime) { }

void RenderingSettingsPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Rendering Mode:");
    if (ImGui::Combo("##rmode", reinterpret_cast<int*>(&renderer->settings.mode), renderModes, IM_ARRAYSIZE(renderModes))) {
        renderer->updateRenderingSettings();
        surfaceMesher->updateSettings();
    }

    ImGui::Spacing();
    ImGui::Separator();
    switch (renderer->settings.mode) {
    case SOLID_COLOR_PARTICLES:
        ImGui::PushID(SOLID_COLOR_PARTICLES);
        ImGui::Text("Size:");
        if (ImGui::DragFloat("##ps", &renderer->settings.particleSize, 0.0001f, 0.0)) {
            renderer->updateParticleSize();
        }
        ImGui::PopID();
        break;

    case VELOCITY_PARTICLES:
        ImGui::Text("Size:");
        if (ImGui::DragFloat("##ps", &renderer->settings.particleSize, 0.0001f, 0.0)) {
            renderer->updateParticleSize();
        }

        ImGui::Text("Minimum Velocity:");
        if (ImGui::DragFloat("##vpMinV", &renderer->settings.velocityMin, 0.01f))
            renderer->updateRenderingSettings();

        ImGui::Text("Minimum Velocity Color:");
        if (ImGui::ColorEdit4("##vpMinVC", glm::value_ptr(renderer->settings.minVelColor)))
            renderer->updateRenderingSettings();

        ImGui::Text("Maximum Velocity:");
        if (ImGui::DragFloat("##vpMaxV", &renderer->settings.velocityMax, 0.01f))
            renderer->updateRenderingSettings();

        ImGui::Text("Maximum Velocity Color:");
        if (ImGui::ColorEdit4("##vpMaxVC", glm::value_ptr(renderer->settings.maxVelColor)))
            renderer->updateRenderingSettings();
        break;

    case PRESSURE_PARTICLES:
        ImGui::Text("Size:");
        if (ImGui::DragFloat("##ps", &renderer->settings.particleSize, 0.0001f, 0.0)) {
            renderer->updateParticleSize();
        }

        ImGui::Text("Minimum Pressure:");
        if (ImGui::DragFloat("##vpMinP", &renderer->settings.pressureMin, 0.01f))
            renderer->updateRenderingSettings();

        ImGui::Text("Minimum Pressure Color:");
        if (ImGui::ColorEdit4("##vpMinPC", glm::value_ptr(renderer->settings.minPresColor)))
            renderer->updateRenderingSettings();

        ImGui::Text("Maximum Pressure:");
        if (ImGui::DragFloat("##vpMaxP", &renderer->settings.pressureMax, 0.01f))
            renderer->updateRenderingSettings();

        ImGui::Text("Maximum Pressure Color:");
        if (ImGui::ColorEdit4("##vpMaxPC", glm::value_ptr(renderer->settings.maxPresColor)))
            renderer->updateRenderingSettings();
        break;

    case SURFACE_PARTICLES:
        ImGui::Text("Size:");
        if (ImGui::DragFloat("##ps", &renderer->settings.particleSize, 0.0001f, 0.0)) {
            renderer->updateParticleSize();
        }

        ImGui::Text("Maximum Surface Tension:");
        if (ImGui::DragFloat("##mst", &renderer->settings.maxSurfaceTension, 0.01f))
            renderer->updateRenderingSettings();
        break;

    case MARCHING_CUBES:
        ImGui::PushID(MARCHING_CUBES);
        ImGui::Text("Color:");
        if (ImGui::ColorEdit4("##mcColor", glm::value_ptr(surfaceMesher->settings.color))) {
            surfaceMesher->updateSettings();
        }

        ImGui::Text("Resolution:");
        if (ImGui::DragInt3("##mcResolution", glm::value_ptr(surfaceMesher->settings.resolution), 1, 1, 1024, "%d", ImGuiSliderFlags_AlwaysClamp)) {
            surfaceMesher->updateVoxelFieldSize();
        }

        ImGui::Text("Iso Level:");
        if (ImGui::DragFloat("##mcIsoLevel", &surfaceMesher->settings.isoLevel, 0.001f, -0.499f, 0.5f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
            surfaceMesher->updateSettings();
        }

        if (ImGui::Checkbox("Smoooth Normals##mcSmoothNormals", &surfaceMesher->settings.smoothNormals)) {
            surfaceMesher->updateSettings();
        }

        ImGui::PopID();
        break;
    }

    ImGui::End();
}
