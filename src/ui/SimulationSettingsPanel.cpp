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

SimulationSettingsPanel::SimulationSettingsPanel()
    : UIPanel("Simulation Settings") {
    simulation  = FluidSimulationGPU::getInstance();
    renderer    = FluidRenderer::getInstance();
}

void SimulationSettingsPanel::update(double deltaTime) { }

void SimulationSettingsPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    if (ImGui::CollapsingHeader("Domain Settings")) {
        ImGui::PushID("Domain");
        ImGui::Text("X:");
        if (ImGui::DragFloat2("##domainX", glm::value_ptr(simulation->settings.xDomain), 0.01f)) {
            renderer->updateDomainModelMatrix();
        }

        ImGui::Text("Y:");
        if (ImGui::DragFloat2("##domainY", glm::value_ptr(simulation->settings.yDomain), 0.01f)) {
            renderer->updateDomainModelMatrix();
        }

        ImGui::Text("Z:");
        if (ImGui::DragFloat2("##domainZ", glm::value_ptr(simulation->settings.zDomain), 0.01f)) {
            renderer->updateDomainModelMatrix();
        }
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader("Particle Settings")) {
        ImGui::Text("H:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Size of Kernel");
        ImGui::DragFloat("##H", &simulation->settings.h, 0.0001, 0.0001);

        ImGui::Text("P_0:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Rest Density");
        ImGui::DragFloat("##P_0", &simulation->settings.p_0, 0.001f);

        ImGui::Text("K:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Stiffness");
        ImGui::DragFloat("##k", &simulation->settings.k, 0.001f);

        ImGui::Text("Mu:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Viscosity Coefficient");
        ImGui::DragFloat("##mu", &simulation->settings.mu, 0.001f);

        ImGui::Text("Surface Tension Threshold:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Surface Tension Threshold");
        ImGui::DragFloat("##TT", &simulation->settings.tensionThreshold, 0.001f);

        ImGui::Text("Surface Tension:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Surface Tension Strength");
        ImGui::DragFloat("##T", &simulation->settings.tension, 0.001f);
    }

    if (ImGui::CollapsingHeader("Gravity Settings")) {
        ImGui::Checkbox("Enable##g", &simulation->settings.enableGravity);

        ImGui::Text("Gravity:");
        ImGui::DragFloat("##g", &simulation->settings.gravity, 0.01);
    }

    if (ImGui::CollapsingHeader("CFL Settings")) {
        ImGui::Checkbox("Enable##cfl", &simulation->settings.useCFL);

        ImGui::Text("Safety Factor:");
        ImGui::DragFloat("##cflSF", &simulation->settings.safetyFactor, 0.01);
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

    json["h"]                   = simulation->settings.h;
    json["p_0"]                 = simulation->settings.p_0;
    json["k"]                   = simulation->settings.k;
    json["mu"]                  = simulation->settings.mu;
    json["tensionThreshold"]    = simulation->settings.tensionThreshold;
    json["tension"]             = simulation->settings.tension;
    json["enableGravity"]       = simulation->settings.enableGravity;
    json["gravity"]             = simulation->settings.gravity;

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

    simulation->settings.h                  = json.value("h", simulation->settings.h);
    simulation->settings.p_0                = json.value("p_0", simulation->settings.p_0);
    simulation->settings.k                  = json.value("k", simulation->settings.k);
    simulation->settings.mu                 = json.value("mu", simulation->settings.mu);
    simulation->settings.tensionThreshold   = json.value("tensionThreshold", simulation->settings.tensionThreshold);
    simulation->settings.tension            = json.value("tension", simulation->settings.tension);
    simulation->settings.enableGravity      = json.value("enableGravity", simulation->settings.enableGravity);
    simulation->settings.gravity            = json.value("gravity", simulation->settings.gravity);
}
