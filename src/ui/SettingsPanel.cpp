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
    : UIPanel(name), renderer(renderer), simulation(simulation) {
    settings = &simulation->settings;
}

void SettingsPanel::update(double deltaTime) { }

void SettingsPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());

    if (ImGui::CollapsingHeader("Particle Settings")) {
        if (ImGui::InputFloat("H", &settings->h)) {
            simulation->updateBoundaries();
        }
        ImGui::InputFloat("P_0", &settings->p_0);
        ImGui::InputFloat("k", &settings->k);
        ImGui::InputFloat("mu", &settings->mu);
        ImGui::InputFloat("Tension Threshold", &settings->tensionThreshold);
        ImGui::InputFloat("Tension", &settings->tension);
    }

    if (ImGui::CollapsingHeader("Gravity Settings")) {
        ImGui::Checkbox("Enable", &settings->enableGravity);
        ImGui::InputFloat("Gravity", &settings->gravity);
    }

    if (ImGui::CollapsingHeader("Boundary Settings")) {
        if (ImGui::DragFloat2("X", glm::value_ptr(settings->xBoundary), 0.01f)) {
            simulation->updateBoundaries();
            renderer->rebuildBoundaryBox();
        }
        if (ImGui::DragFloat2("Y", glm::value_ptr(settings->yBoundary), 0.01f)) {
            simulation->updateBoundaries();
            renderer->rebuildBoundaryBox();
        }
        if (ImGui::DragFloat2("Z", glm::value_ptr(settings->zBoundary), 0.01f)) {
            simulation->updateBoundaries();
            renderer->rebuildBoundaryBox();
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

    json["h"] = settings->h;
    json["p_0"] = settings->p_0;
    json["k"] = settings->k;
    json["mu"] = settings->mu;
    json["tensionThreshold"] = settings->tensionThreshold;
    json["tension"] = settings->tension;
    json["enableGravity"] = settings->enableGravity;
    json["gravity"] = settings->gravity;

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

    settings->h = json.value("h", settings->h);
    settings->p_0 = json.value("p_0", settings->p_0);
    settings->k = json.value("k", settings->k);
    settings->mu = json.value("mu", settings->mu);
    settings->tensionThreshold = json.value("tensionThreshold", settings->tensionThreshold);
    settings->tension = json.value("tension", settings->tension);
    settings->enableGravity = json.value("enableGravity", settings->enableGravity);
    settings->gravity = json.value("gravity", settings->gravity);
}
