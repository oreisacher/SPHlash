//
// Created by ruut on 4/10/26.
//

#include "InitPanel.h"

#include <format>
#include <iostream>
#include <ostream>

#include "imgui_internal.h"
#include "glm/gtc/type_ptr.inl"

InitPanel::InitPanel()
    : UIPanel("Init Settings")
{
    simulation = FluidSimulationGPU::getInstance();
    renderer = FluidRenderer::getInstance();

    recalculateParticles();
}

void InitPanel::update(double deltaTime) { }

void InitPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    ImGui::SeparatorText("Particles");

    ImGui::Text("Total Mass:");
    if (ImGui::DragFloat("##tm", &simulation->initSettings.totalMass, 10)) {
        recalculateParticles();
    }

    ImGui::Text("Particle Spacing:");
    if (ImGui::DragFloat("##ps", &simulation->initSettings.particleSpacing, 0.001)) {
        recalculateParticles();
    }

    ImGui::SeparatorText("Area");

    ImGui::Text("X:");
    if (ImGui::DragFloat2("##saX", glm::value_ptr(simulation->initSettings.xBoundary), 0.01f)) {
        recalculateParticles();
        renderer->rebuildSpawnAreaBox();
    }

    ImGui::Text("Y:");
    if (ImGui::DragFloat2("##saY", glm::value_ptr(simulation->initSettings.yBoundary), 0.01f)) {
        recalculateParticles();
        renderer->rebuildSpawnAreaBox();
    }

    ImGui::Text("Z:");
    if (ImGui::DragFloat2("##saZ", glm::value_ptr(simulation->initSettings.zBoundary), 0.01f)) {
        recalculateParticles();
        renderer->rebuildSpawnAreaBox();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginTable("stats", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Particles:");
        ImGui::TableNextColumn();
        ImGui::Text("%d", particlesToSpawn);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Volume:");
        ImGui::TableNextColumn();
        ImGui::Text("%.6f", spawnVolume);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Mass Per Particle:");
        ImGui::TableNextColumn();
        ImGui::Text("%.8f", massPerParticle);

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Spawn Particles", ImVec2(totalWidth, 0))) {
        simulation->initParticles();
        renderer->updateParticleData();
    }

    ImGui::End();
}

void InitPanel::recalculateParticles() {
    float width         = simulation->initSettings.xBoundary.y - simulation->initSettings.xBoundary.x;
    float height        = simulation->initSettings.yBoundary.y - simulation->initSettings.yBoundary.x;
    float depth         = simulation->initSettings.zBoundary.y - simulation->initSettings.zBoundary.x;

    int countX = std::floor(width  / simulation->initSettings.particleSpacing + 1e-5) + 1;
    int countY = std::floor(height / simulation->initSettings.particleSpacing + 1e-5) + 1;
    int countZ = std::floor(depth  / simulation->initSettings.particleSpacing + 1e-5) + 1;

    particlesToSpawn = countX * countY * countZ;
    spawnVolume = width * height * depth;
    massPerParticle = simulation->initSettings.totalMass * (spawnVolume / particlesToSpawn);
}
