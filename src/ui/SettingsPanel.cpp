//
// Created by ruut on 3/16/26.
//

#include "SettingsPanel.h"

#include <fstream>

#include "imgui.h"
#include "nlohmann/json.hpp"
#include "ImGuiFileDialog.h"
#include "glm/gtc/type_ptr.inl"
#include "simulation/FluidSimulationGPU.h"

SettingsPanel::SettingsPanel(const std::string& name, FluidSimulationGPU* simulation, FluidRenderer* renderer)
    : UIPanel(name), simulation(simulation), renderer(renderer) {
    simulationSettings = &simulation->settings;
    renderSettings = &renderer->settings;
}

void SettingsPanel::update(double deltaTime) { }

void SettingsPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());

    if (ImGui::CollapsingHeader("Particle Settings")) {
        ImGui::DragFloat("H", &simulationSettings->h, 0.0001, 0.0001);
        ImGui::DragFloat("P_0", &simulationSettings->p_0, 0.001f);
        ImGui::DragFloat("k", &simulationSettings->k, 0.001f);
        ImGui::DragFloat("mu", &simulationSettings->mu, 0.001f);
        ImGui::DragFloat("Tension Threshold", &simulationSettings->tensionThreshold, 0.001f);
        ImGui::DragFloat("Tension", &simulationSettings->tension, 0.001f);
    }

    if (ImGui::CollapsingHeader("Gravity Settings")) {
        ImGui::Checkbox("Enable", &simulationSettings->enableGravity);
        ImGui::InputFloat("Gravity", &simulationSettings->gravity);
    }

    if (ImGui::CollapsingHeader("Boundary Settings")) {
        if (ImGui::DragFloat2("X", glm::value_ptr(simulationSettings->xBoundary), 0.01f)) {
            renderer->rebuildBoundaryBox();
        }
        if (ImGui::DragFloat2("Y", glm::value_ptr(simulationSettings->yBoundary), 0.01f)) {
            renderer->rebuildBoundaryBox();
        }
        if (ImGui::DragFloat2("Z", glm::value_ptr(simulationSettings->zBoundary), 0.01f)) {
            renderer->rebuildBoundaryBox();
        }
    }

    if (ImGui::CollapsingHeader("Rendering Settings")) {
        if (ImGui::DragFloat("Particle Size", &renderSettings->particleSize, 0.0001f)) {
            renderer->updateParticleSize();
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Save Settings")) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("SaveLocation", "Choose location", ".json", config);
    }

    if (ImGuiFileDialog::Instance()->Display("SaveLocation")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            SaveSettings(filePathName);
        }

        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::SameLine();

    if (ImGui::Button("Load Settings")) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("LoadLocation", "Choose location", ".json", config);
    }

    if (ImGuiFileDialog::Instance()->Display("LoadLocation")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            LoadSettings(filePathName);
        }

        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::Separator();

    if (ImGui::Button("Reset Particles")) {
        simulation->reset();
    }

    ImGui::End();
}

void SettingsPanel::SaveSettings(const std::string& path) const {
    nlohmann::json json;

    json["h"] = simulationSettings->h;
    json["p_0"] = simulationSettings->p_0;
    json["k"] = simulationSettings->k;
    json["mu"] = simulationSettings->mu;
    json["tensionThreshold"] = simulationSettings->tensionThreshold;
    json["tension"] = simulationSettings->tension;
    json["enableGravity"] = simulationSettings->enableGravity;
    json["gravity"] = simulationSettings->gravity;

    // Write safe file to disk
    std::ofstream file(path);
    file << json.dump(4);
}

void SettingsPanel::LoadSettings(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        return;

    nlohmann::json json;
    file >> json;

    simulationSettings->h = json.value("h", simulationSettings->h);
    simulationSettings->p_0 = json.value("p_0", simulationSettings->p_0);
    simulationSettings->k = json.value("k", simulationSettings->k);
    simulationSettings->mu = json.value("mu", simulationSettings->mu);
    simulationSettings->tensionThreshold = json.value("tensionThreshold", simulationSettings->tensionThreshold);
    simulationSettings->tension = json.value("tension", simulationSettings->tension);
    simulationSettings->enableGravity = json.value("enableGravity", simulationSettings->enableGravity);
    simulationSettings->gravity = json.value("gravity", simulationSettings->gravity);
}
