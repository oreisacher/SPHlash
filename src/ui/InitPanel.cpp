//
// Created by ruut on 4/10/26.
//

#include "InitPanel.h"

#include <format>
#include <iostream>
#include <ostream>

#include "imgui_internal.h"
#include "glm/gtc/type_ptr.inl"

InitPanel::InitPanel(const std::string& name, FluidSimulationGPU* simulation, FluidRenderer* renderer)
    : UIPanel(name), simulation(simulation), renderer(renderer)
{
    recalculateParticles();
}

void InitPanel::update(double deltaTime) {
}

void InitPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());

    ImGui::Text("Particles");
    ImGui::DragFloat("Total mass", &simulation->initSettings.totalMass, 10);
    if (ImGui::DragFloat("Particle spacing", &simulation->initSettings.particleSpacing, 0.001)) {
        recalculateParticles();
    }

    ImGui::Text("Spawn Area");
    if (ImGui::DragFloat2("X", glm::value_ptr(simulation->initSettings.xBoundary), 0.01f)) {
        recalculateParticles();
        renderer->rebuildSpawnAreaBox();
    }
    if (ImGui::DragFloat2("Y", glm::value_ptr(simulation->initSettings.yBoundary), 0.01f)) {
        recalculateParticles();
        renderer->rebuildSpawnAreaBox();
    }
    if (ImGui::DragFloat2("Z", glm::value_ptr(simulation->initSettings.zBoundary), 0.01f)) {
        recalculateParticles();
        renderer->rebuildSpawnAreaBox();
    }

    ImGui::Separator();

    ImGui::Text(std::format("Particles: {}", particlesToSpawn).c_str());
    ImGui::Text(std::format("Volume: {}", spawnVolume).c_str());
    ImGui::Text(std::format("Mass per particle: {}", massPerParticle).c_str());

    ImGui::Separator();

    if (ImGui::Button("Spawn Particles")) {
        simulation->initParticles();
        renderer->updateParticleData();
    }

    ImGui::End();
}

void InitPanel::recalculateParticles() {
    float width         = simulation->initSettings.xBoundary.y - simulation->initSettings.xBoundary.x;
    float height        = simulation->initSettings.yBoundary.y - simulation->initSettings.yBoundary.x;
    float depth         = simulation->initSettings.zBoundary.y - simulation->initSettings.zBoundary.x;

    spawnVolume = width * height * depth;
    massPerParticle = simulation->initSettings.totalMass * (spawnVolume / simulation->particleCount);

    int countX = std::floor(width  / simulation->initSettings.particleSpacing + 1e-5) + 1;
    int countY = std::floor(height / simulation->initSettings.particleSpacing + 1e-5) + 1;
    int countZ = std::floor(depth  / simulation->initSettings.particleSpacing + 1e-5) + 1;

    particlesToSpawn = countX * countY * countZ;
}
