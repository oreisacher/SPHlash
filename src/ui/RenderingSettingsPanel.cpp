//
// Created by ruut on 4/12/26.
//

#include "RenderingSettingsPanel.h"

#include <iostream>

#include "imgui.h"
#include "glm/gtc/type_ptr.hpp"

RenderingSettingsPanel::RenderingSettingsPanel(ParticlesNode* particles)
    : UIPanel("Rendering Settings"), surfaceMesher(particles->getSurfaceMesher()), particles(particles)
{ }

void RenderingSettingsPanel::update(double deltaTime) { }

void RenderingSettingsPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Rendering Mode:");
    if (ImGui::Combo("##rmode", reinterpret_cast<int*>(&particles->settings.mode), renderModes, IM_ARRAYSIZE(renderModes))) {
        particles->updateRenderingSettings();
        // surfaceMesher->updateSettings();
    }

    ImGui::Spacing();
    ImGui::Separator();
    switch (particles->settings.mode) {
    case SOLID_COLOR_PARTICLES:
        ImGui::PushID(SOLID_COLOR_PARTICLES);
        ImGui::Text("Size:");
        if (ImGui::DragFloat("##ps", &particles->settings.particleSize, 0.0001f, 0.0))
            particles->updateRenderingSettings();
        ImGui::PopID();
        break;

    case VELOCITY_PARTICLES:
        ImGui::Text("Size:");
        if (ImGui::DragFloat("##ps", &particles->settings.particleSize, 0.0001f, 0.0))
            particles->updateRenderingSettings();

        ImGui::Text("Minimum Velocity:");
        if (ImGui::DragFloat("##vpMinV", &particles->settings.velocityMin, 0.01f))
            particles->updateRenderingSettings();

        ImGui::Text("Minimum Velocity Color:");
        if (ImGui::ColorEdit4("##vpMinVC", glm::value_ptr(particles->settings.minVelColor)))
            particles->updateRenderingSettings();

        ImGui::Text("Maximum Velocity:");
        if (ImGui::DragFloat("##vpMaxV", &particles->settings.velocityMax, 0.01f))
            particles->updateRenderingSettings();

        ImGui::Text("Maximum Velocity Color:");
        if (ImGui::ColorEdit4("##vpMaxVC", glm::value_ptr(particles->settings.maxVelColor)))
            particles->updateRenderingSettings();
        break;

    case PRESSURE_PARTICLES:
        ImGui::Text("Size:");
        if (ImGui::DragFloat("##ps", &particles->settings.particleSize, 0.0001f, 0.0))
            particles->updateRenderingSettings();

        ImGui::Text("Minimum Pressure:");
        if (ImGui::DragFloat("##vpMinP", &particles->settings.pressureMin, 0.01f))
            particles->updateRenderingSettings();

        ImGui::Text("Minimum Pressure Color:");
        if (ImGui::ColorEdit4("##vpMinPC", glm::value_ptr(particles->settings.minPresColor)))
            particles->updateRenderingSettings();

        ImGui::Text("Maximum Pressure:");
        if (ImGui::DragFloat("##vpMaxP", &particles->settings.pressureMax, 0.01f))
            particles->updateRenderingSettings();

        ImGui::Text("Maximum Pressure Color:");
        if (ImGui::ColorEdit4("##vpMaxPC", glm::value_ptr(particles->settings.maxPresColor)))
            particles->updateRenderingSettings();
        break;

    case SURFACE_PARTICLES:
        ImGui::Text("Size:");
        if (ImGui::DragFloat("##ps", &particles->settings.particleSize, 0.0001f, 0.0))
            particles->updateRenderingSettings();

        ImGui::Text("Maximum Surface Tension:");
        if (ImGui::DragFloat("##mst", &particles->settings.maxSurfaceTension, 0.01f))
            particles->updateRenderingSettings();
        break;

    case MARCHING_CUBES:
        ImGui::PushID(MARCHING_CUBES);
        ImGui::Text("Color:");
        if (ImGui::ColorEdit4("##mcColor", glm::value_ptr(surfaceMesher->settings.color)))
            surfaceMesher->updateSettings();

        ImGui::Text("Resolution:");
        if (ImGui::DragInt3("##mcResolution", glm::value_ptr(surfaceMesher->settings.resolution), 1, 1, 1024, "%d", ImGuiSliderFlags_AlwaysClamp))
            surfaceMesher->updateVoxelFieldSize();

        ImGui::Text("Iso Level:");
        if (ImGui::DragFloat("##mcIsoLevel", &surfaceMesher->settings.isoLevel, 0.001f, -0.499f, 0.5f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
            surfaceMesher->updateSettings();

        if (ImGui::Checkbox("Smoooth Normals##mcSmoothNormals", &surfaceMesher->settings.smoothNormals))
            surfaceMesher->updateSettings();

        ImGui::PopID();
        break;

    case SCREEN_SPACE:
        ImGui::PushID(SCREEN_SPACE);
        ImGui::Text("Size:");
        if (ImGui::DragFloat("##bs", &particles->settings.billboardSize, 0.0001f, 0.0))
            particles->updateRenderingSettings();

        ImGui::PopID();
        break;
    }

    ImGui::End();
}