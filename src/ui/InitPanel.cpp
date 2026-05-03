//
// Created by ruut on 4/10/26.
//

#include "InitPanel.h"

#include <format>
#include <iostream>
#include "imgui_internal.h"
#include "glm/gtc/type_ptr.inl"

InitPanel::InitPanel(Editor* editor, FluidSurfaceMesher* mesher)
    : UIPanel("Init Settings"), editor(editor), mesher(mesher)
{
    simulation = FluidSimulation::getInstance();
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

    ImGui::Text("Center: ");
    if (ImGui::DragFloat3("##saCenter", glm::value_ptr(simulation->initSettings.spawnArea.center), 0.01f)) {
        recalculateParticles();
        editor->updateSpawnAreaTransform();
    }

    ImGui::Text("Size: ");
    if (ImGui::DragFloat3("##saSize", glm::value_ptr(simulation->initSettings.spawnArea.size), 0.01f)) {
        recalculateParticles();
        editor->updateSpawnAreaTransform();
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
        mesher->generateMesh(true);
    }

    ImGui::End();
}

void InitPanel::recalculateParticles() {
    const glm::vec3 size = simulation->initSettings.spawnArea.size;
    int countX = std::floor(size.x / simulation->initSettings.particleSpacing + 1e-5) + 1;
    int countY = std::floor(size.y / simulation->initSettings.particleSpacing + 1e-5) + 1;
    int countZ = std::floor(size.z / simulation->initSettings.particleSpacing + 1e-5) + 1;

    particlesToSpawn = countX * countY * countZ;
    spawnVolume = simulation->initSettings.spawnArea.volume();

    // TODO - Check if correct
    massPerParticle = simulation->initSettings.totalMass * (spawnVolume / particlesToSpawn);
}
