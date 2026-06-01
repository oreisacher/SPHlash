//
// Created by ruut on 3/24/26.
//

#include "ParticlesPanel.h"
#include <format>
#include "imgui.h"
#include "simulation/FluidSimulation.h"

ParticlesPanel::ParticlesPanel(FluidSimulation* simulation, FluidRenderer* renderer)
    : UIPanel("Particles"), simulation(simulation), renderer(renderer)
{ }

void ParticlesPanel::update(double deltaTime) {
    time += deltaTime;

    if (time >= 0.5f) {
        time = 0.0f;

        // Fetch new data from simulation
        particle = simulation->getParticle(selectedParticle);
    }
}

void ParticlesPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());

    int particleCount = simulation->getParticleCount();

    if (particleCount == 0) {
        auto windowSize = ImGui::GetWindowSize();
        auto textSize   = ImGui::CalcTextSize("No particles.");

        ImGui::SetCursorPos({
            (windowSize.x - textSize.x) * 0.5f,
            (windowSize.y - textSize.y) * 0.5f
        });
        ImGui::Text("No particles.");
    } else {
        ImGui::Checkbox("Show particle info", &showParticles);

        if (showParticles) {
            ImGui::Spacing();
            ImGui::PushItemWidth(-FLT_MIN);
            ImGui::Text("Particle ID:");
            ImGui::DragInt("##id", &selectedParticle, 1, 0, particleCount - 1, "%d", ImGuiSliderFlags_AlwaysClamp);

            renderer->getParticleShader()->bind();
            renderer->getParticleShader()->setInt("selectedParticleID", selectedParticle);

            if (particle.properties.w != -1.0) {
                float totalWidth = ImGui::GetContentRegionAvail().x;
                float spacing = ImGui::GetStyle().ItemSpacing.x;
                float itemWidth = (totalWidth - 2.0f * spacing) / 3.0f;

                ImGui::Separator();

                ImGui::Text("Position:");
                ImGui::Button(std::format("{:.3f}##posx", particle.position.x).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##posy", particle.position.y).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##posz", particle.position.z).c_str(), ImVec2(itemWidth, 0));

                ImGui::Text("Velocity:");
                ImGui::Button(std::format("{:.3f}##velx", particle.velocity.x).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##vely", particle.velocity.y).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##velz", particle.velocity.z).c_str(), ImVec2(itemWidth, 0));

                ImGui::Separator();

                ImGui::Text("Mass:");
                ImGui::Button(std::format("{:.3f}##mass", particle.properties.x).c_str(), ImVec2(-FLT_MIN, 0));

                ImGui::Text("Density:");
                ImGui::Button(std::format("{:.3f}##dens", particle.properties.y).c_str(), ImVec2(-FLT_MIN, 0));

                ImGui::Text("Pressure:");
                ImGui::Button(std::format("{:.3f}##pres", particle.properties.z).c_str(), ImVec2(-FLT_MIN, 0));

                ImGui::Separator();

                ImGui::Text("Pressure Force:");
                ImGui::Button(std::format("{:.3f}##pfx", particle.pressureForce.x).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##pfy", particle.pressureForce.y).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##pfz", particle.pressureForce.z).c_str(), ImVec2(itemWidth, 0));

                ImGui::Text("Viscosity Force:");
                ImGui::Button(std::format("{:.3f}##vfx", particle.viscosityForce.x).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##vfy", particle.viscosityForce.y).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##vfz", particle.viscosityForce.z).c_str(), ImVec2(itemWidth, 0));

                ImGui::Text("Surface Tension Force:");
                ImGui::Button(std::format("{:.3f}##stfx", particle.surfaceTensionForce.x).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##stfy", particle.surfaceTensionForce.y).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##stfz", particle.surfaceTensionForce.z).c_str(), ImVec2(itemWidth, 0));
            } else {
                selectedParticle = 0;
            }
        } else {
            renderer->getParticleShader()->bind();
            renderer->getParticleShader()->setInt("selectedParticleID", -1);
        }
    }

    ImGui::End();
}
