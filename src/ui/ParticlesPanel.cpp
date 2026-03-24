//
// Created by ruut on 3/24/26.
//

#include "ParticlesPanel.h"

#include <format>

#include "imgui.h"
#include "simulation/FluidSimulationGPU.h"

ParticlesPanel::ParticlesPanel(const std::string& name, GLuint particlesVBO, int particleCount)
    : UIPanel(name), particlesVBO(particlesVBO), particleCount(particleCount) { }

void ParticlesPanel::update(double deltaTime) { }

void ParticlesPanel::draw() {
    if (!visible)
        return;

    // Bind particle data

    ImGui::Begin(name.c_str());

    ImGui::Checkbox("Show particle info", &showParticles);

    if (showParticles) {
        ImGui::InputInt("Particle", &selectedParticle);

        glBindBuffer(GL_ARRAY_BUFFER, particlesVBO);
        ParticleGPU* data = static_cast<ParticleGPU*>(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));

        ImGui::Text(std::format("Position: {:.3f} {:.3f} {:.3f}", data[selectedParticle].position.x, data[selectedParticle].position.y, data[selectedParticle].position.z).c_str());
        ImGui::Text(std::format("Velocity: {:.3f} {:.3f} {:.3f}", data[selectedParticle].velocity.x, data[selectedParticle].velocity.y, data[selectedParticle].velocity.z).c_str());
        ImGui::Separator();
        ImGui::Text(std::format("Mass: {:.3f}", data[selectedParticle].properties.x).c_str());
        ImGui::Text(std::format("Density: {}", data[selectedParticle].properties.y).c_str());
        ImGui::Text(std::format("Pressure: {}", data[selectedParticle].properties.z).c_str());
        ImGui::Separator();
        ImGui::Text(std::format("Pressure Force: {:.3f} {:.3f} {:.3f}", data[selectedParticle].pressureForce.x, data[selectedParticle].pressureForce.y, data[selectedParticle].pressureForce.z).c_str());
        ImGui::Text(std::format("Viscosity Force: {:.3f} {:.3f} {:.3f}", data[selectedParticle].viscosityForce.x, data[selectedParticle].viscosityForce.y, data[selectedParticle].viscosityForce.z).c_str());
        ImGui::Text(std::format("Surface Tension Force: {:.3f} {:.3f} {:.3f}", data[selectedParticle].surfaceTensionForce.x, data[selectedParticle].surfaceTensionForce.y, data[selectedParticle].surfaceTensionForce.z).c_str());

        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    ImGui::End();
}
