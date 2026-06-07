//
// Created by ruut on 4/12/26.
//

#include "RenderingSettingsPanel.h"

#include <iostream>

#include "imgui.h"
#include "glm/gtc/type_ptr.hpp"

RenderingSettingsPanel::RenderingSettingsPanel(ParticlesNode* particles)
    : UIPanel("Rendering Settings"), particles(particles)
{
    surfaceMesher = particles->getSurfaceMesher();
    surfaceScreenSpace = particles->getSurfaceScreenSpace();
}

void RenderingSettingsPanel::update(double deltaTime) { }

void RenderingSettingsPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);

    ImGui::Text("Rendering Mode:");
    if (ImGui::Combo("##rmode", reinterpret_cast<int*>(&particles->settings.mode), renderModes, IM_ARRAYSIZE(renderModes)))
        particles->updateRenderingSettings();

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
        if (ImGui::ColorEdit3("##vpMinVC", glm::value_ptr(particles->settings.minVelColor)))
            particles->updateRenderingSettings();

        ImGui::Text("Maximum Velocity:");
        if (ImGui::DragFloat("##vpMaxV", &particles->settings.velocityMax, 0.01f))
            particles->updateRenderingSettings();

        ImGui::Text("Maximum Velocity Color:");
        if (ImGui::ColorEdit3("##vpMaxVC", glm::value_ptr(particles->settings.maxVelColor)))
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
        if (ImGui::ColorEdit3("##vpMinPC", glm::value_ptr(particles->settings.minPresColor)))
            particles->updateRenderingSettings();

        ImGui::Text("Maximum Pressure:");
        if (ImGui::DragFloat("##vpMaxP", &particles->settings.pressureMax, 0.01f))
            particles->updateRenderingSettings();

        ImGui::Text("Maximum Pressure Color:");
        if (ImGui::ColorEdit3("##vpMaxPC", glm::value_ptr(particles->settings.maxPresColor)))
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

        ImGui::Text("Resolution:");
        if (ImGui::DragInt3("##mcResolution", glm::value_ptr(surfaceMesher->settings.resolution), 1, 1, 1024, "%d", ImGuiSliderFlags_AlwaysClamp))
            surfaceMesher->updateVoxelFieldSize();

        ImGui::Text("Iso Level:");
        if (ImGui::DragFloat("##mcIsoLevel", &surfaceMesher->settings.isoLevel, 0.001f, -0.499f, 0.5f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
            surfaceMesher->updateSettings();

        if (ImGui::Checkbox("Smoooth Normals##mcSmoothNormals", &surfaceMesher->settings.smoothNormals))
            surfaceMesher->updateSettings();

        ImGui::Text("Color:");
        if (ImGui::ColorEdit3("##mcColor", glm::value_ptr(surfaceMesher->settings.fluidColor)))
            surfaceMesher->updateSettings();

        ImGui::Text("Ambient Strength:");
        if (ImGui::DragFloat("##msAS", &surfaceMesher->settings.ambientStrength, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            surfaceMesher->updateSettings();

        ImGui::Text("Specular Strength:");
        if (ImGui::DragFloat("##mcSS", &surfaceMesher->settings.specularStrength, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            surfaceMesher->updateSettings();

        ImGui::Text("Fresnel Strength:");
        if (ImGui::DragFloat("##mcFS", &surfaceMesher->settings.fresnelStrength, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            surfaceMesher->updateSettings();

        ImGui::PopID();
        break;

    case SCREEN_SPACE:
        ImGui::PushID(SCREEN_SPACE);
        ImGui::Text("Size:");
        if (ImGui::DragFloat("##bs", &surfaceScreenSpace->settings.particleSize, 0.0001f, 0.0))
            surfaceScreenSpace->updateRenderingSettings();

        ImGui::Text("Blur Size:");
        if (ImGui::DragInt("##ssBs", &surfaceScreenSpace->settings.blurSize))
            surfaceScreenSpace->updateRenderingSettings();

        ImGui::Text("Depth Threshold:");
        if (ImGui::DragFloat("##ssDt", &surfaceScreenSpace->settings.depthThreshold, 0.001f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
            surfaceScreenSpace->updateRenderingSettings();

        ImGui::Text("Color:");
        if (ImGui::ColorEdit3("##sscolor", glm::value_ptr(surfaceScreenSpace->settings.fluidColor)))
            surfaceScreenSpace->updateRenderingSettings();

        ImGui::Text("Transparency:");
        if (ImGui::DragFloat("##ssTrans", &surfaceScreenSpace->settings.transparency, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            surfaceScreenSpace->updateRenderingSettings();

        ImGui::Text("Ambient Strength:");
        if (ImGui::DragFloat("##ssAS", &surfaceScreenSpace->settings.ambientStrength, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            surfaceScreenSpace->updateRenderingSettings();

        ImGui::Text("Specular Strength:");
        if (ImGui::DragFloat("##ssSS", &surfaceScreenSpace->settings.specularStrength, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            surfaceScreenSpace->updateRenderingSettings();

        ImGui::Text("Fresnel Strength:");
        if (ImGui::DragFloat("##ssFS", &surfaceScreenSpace->settings.fresnelStrength, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            surfaceScreenSpace->updateRenderingSettings();

        ImGui::PopID();
        break;
    }

    ImGui::End();
}