//
// Created by ruut on 3/16/26.
//

#include "SimulationSettingsPanel.h"

#include <fstream>

#include "imgui.h"
#include "nlohmann/json.hpp"
#include "ImGuiFileDialog.h"
#include "glm/gtc/type_ptr.inl"
#include "simulation/FluidSimulationGPU.h"

SimulationSettingsPanel::SimulationSettingsPanel(const std::string& name, FluidSimulationGPU* simulation, FluidRenderer* renderer)
    : UIPanel(name), simulation(simulation), renderer(renderer) {
    simulationSettings = &simulation->settings;
    renderSettings = &renderer->settings;
}

void SimulationSettingsPanel::update(double deltaTime) { }

void SimulationSettingsPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    if (ImGui::CollapsingHeader("Particle Settings")) {
        ImGui::Text("H:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Size of Kernel");
        ImGui::DragFloat("##H", &simulationSettings->h, 0.0001, 0.0001);

        ImGui::Text("P_0:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Rest Density");
        ImGui::DragFloat("##P_0", &simulationSettings->p_0, 0.001f);

        ImGui::Text("K:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Stiffness");
        ImGui::DragFloat("##k", &simulationSettings->k, 0.001f);

        ImGui::Text("Mu:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Viscosity Coefficient");
        ImGui::DragFloat("##mu", &simulationSettings->mu, 0.001f);

        ImGui::Text("Surface Tension Threshold:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Surface Tension Threshold");
        ImGui::DragFloat("##TT", &simulationSettings->tensionThreshold, 0.001f);

        ImGui::Text("Surface Tension:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Surface Tension Strength");
        ImGui::DragFloat("##T", &simulationSettings->tension, 0.001f);
    }

    if (ImGui::CollapsingHeader("Gravity Settings")) {
        ImGui::Checkbox("Enable##g", &simulationSettings->enableGravity);

        ImGui::Text("Gravity:");
        ImGui::DragFloat("##g", &simulationSettings->gravity, 0.01);
    }

    if (ImGui::CollapsingHeader("CFL Settings")) {
        ImGui::Checkbox("Enable##cfl", &simulationSettings->useCFL);

        ImGui::Text("Safety Factor:");
        ImGui::DragFloat("##cflSF", &simulationSettings->safetyFactor, 0.01);
    }

    ImGui::Separator();

    if (ImGui::Button("Save Settings", ImVec2(totalWidth, 0))) {
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

    if (ImGui::Button("Load Settings", ImVec2(totalWidth, 0))) {
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

    ImGui::End();
}

void SimulationSettingsPanel::SaveSettings(const std::string& path) const {
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

void SimulationSettingsPanel::LoadSettings(const std::string& path) {
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
