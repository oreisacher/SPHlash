//
// Created by ruut on 4/12/26.
//

#include "RenderingSettingsPanel.h"

#include <iostream>

#include "imgui.h"
#include "glm/gtc/type_ptr.hpp"

RenderingSettingsPanel::RenderingSettingsPanel(const std::string& name, FluidRenderer* renderer)
    : UIPanel(name), renderer(renderer)
{ }

void RenderingSettingsPanel::update(double deltaTime) { }

void RenderingSettingsPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    ImGui::Text("Particle Size:");
    if (ImGui::DragFloat("##ps", &renderer->settings.particleSize, 0.0001f, 0.0)) {
        renderer->updateParticleSize();
    }

    ImGui::SeparatorText("Rendering Mode");

    if (ImGui::Combo("##rmode", reinterpret_cast<int*>(&renderer->settings.mode), renderModes, IM_ARRAYSIZE(renderModes))) {
        renderer->updateRenderingSettings();
    }

    switch (renderer->settings.mode) {
    case SOLID_COLOR_PARTICLES:
        ImGui::Text("Color:");
        if (ImGui::ColorEdit3("##scpCE", glm::value_ptr(renderer->settings.solidColor)))
            renderer->updateRenderingSettings();
        break;

    case VELOCITY_PARTICLES:
        ImGui::Text("Minimum Velocity:");
        if (ImGui::DragFloat("##vpMinV", &renderer->settings.velocityMin, 0.01f))
            renderer->updateRenderingSettings();

        ImGui::Text("Minimum Velocity Color:");
        if (ImGui::ColorEdit3("##vpMinVC", glm::value_ptr(renderer->settings.minVelColor)))
            renderer->updateRenderingSettings();

        ImGui::Text("Maximum Velocity:");
        if (ImGui::DragFloat("##vpMaxV", &renderer->settings.velocityMax, 0.01f))
            renderer->updateRenderingSettings();

        ImGui::Text("Maximum Velocity Color:");
        if (ImGui::ColorEdit3("##vpMaxVC", glm::value_ptr(renderer->settings.maxVelColor)))
            renderer->updateRenderingSettings();
        break;

    case PRESSURE_PARTICLES:
        ImGui::Text("Minimum Pressure:");
        if (ImGui::DragFloat("##vpMinP", &renderer->settings.pressureMin, 0.01f))
            renderer->updateRenderingSettings();

        ImGui::Text("Minimum Pressure Color:");
        if (ImGui::ColorEdit3("##vpMinPC", glm::value_ptr(renderer->settings.minPresColor)))
            renderer->updateRenderingSettings();

        ImGui::Text("Maximum Pressure:");
        if (ImGui::DragFloat("##vpMaxP", &renderer->settings.pressureMax, 0.01f))
            renderer->updateRenderingSettings();

        ImGui::Text("Maximum Pressure Color:");
        if (ImGui::ColorEdit3("##vpMaxPC", glm::value_ptr(renderer->settings.maxPresColor)))
            renderer->updateRenderingSettings();
        break;
    }

    ImGui::End();
}
