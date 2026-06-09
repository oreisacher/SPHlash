//
// Created by ruut on 3/16/26.
//

#include "SimulationSettingsPanel.h"

#include <fstream>

#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "UIManager.h"
#include "glm/gtc/type_ptr.inl"
#include "simulation/FluidSimulation.h"

SimulationSettingsPanel::SimulationSettingsPanel(Editor* editor, FluidSimulation* simulation, Renderer* renderer)
    : UIPanel("Simulation Settings"), editor(editor), simulation(simulation), renderer(renderer)
{
    boundaryBuilder = editor->getBoundaryBuilder();
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
        ImGui::DragFloat("##H", &simulation->settings.h, 0.0001, 0.0001);

        ImGui::Text("P_0:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Rest Density");
        ImGui::DragFloat("##P_0", &simulation->settings.p_0, 0.001f);

        ImGui::Text("Max Velocity:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Maximum velocity to expect. Estimate like sqrt(2gH) for example for dam break");
        ImGui::DragFloat("##mv", &simulation->settings.maxVelocity, 0.001f);

        ImGui::Text("Alpha:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Viscosity Coefficient");
        ImGui::DragFloat("##alpha", &simulation->settings.alpha, 0.001f);


        ImGui::Text("Surface Tension:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Surface Tension Strength");
        ImGui::DragFloat("##T", &simulation->settings.surfaceTension, 0.001f);
    }

    if (ImGui::CollapsingHeader("Gravity Settings")) {
        ImGui::Checkbox("Enable##g", &simulation->settings.enableGravity);

        ImGui::Text("Gravity:");
        ImGui::DragFloat("##g", &simulation->settings.gravity, 0.01);
    }

    if (ImGui::CollapsingHeader("Time Settings")) {
        ImGui::Checkbox("Enable fixed Time Step##fixedTimeStep", &simulation->settings.useFixedTimeStep);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("If disabled, the actual frame time is used");

        ImGui::Text("Time Step (ms):");
        ImGui::DragFloat("##fts", &simulation->settings.fixedTimeStep, 0.01);
    }

    if (ImGui::CollapsingHeader("CFL Settings")) {
        ImGui::Checkbox("Enable CFL##cfl", &simulation->settings.useCFL);

        ImGui::Text("Safety Factor:");
        ImGui::DragFloat("##cflSF", &simulation->settings.safetyFactor, 0.01);
    }

    if (ImGui::CollapsingHeader("Boundary Settings")) {
        ImGui::Checkbox("Build on Init##bbInitBuild", &boundaryBuilder->settings.buildOnInit);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Build volume map on simulation init");

        ImGui::Text("Resolution:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Resolution of signed distance field and volume map used for boundaries");
        ImGui::DragInt3("##bbres", glm::value_ptr(boundaryBuilder->settings.resolution), 1, 2, 1024, "%d", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Spacing();

        Hex32Field boundaryField = editor->getBoundaryDensityMap();

        if (ImGui::Button(boundaryField.d_cells ? "Rebuild Boundary Map" : "Build Boundary Map", ImVec2(totalWidth, 0.0f))) {
            if (volumeMapPreview)
                volumeMapPreview->close();

            editor->buildBoundaryDensityMap();
        }

        if (boundaryField.d_cells) {
            if (ImGui::Button("Clear Boundary Map", ImVec2(totalWidth, 0.0f))) {
                if (volumeMapPreview)
                    volumeMapPreview->close();

                editor->clearBoundaryDensityMap();
            }

            if (ImGui::Button(volumeMapPreview ? "Hide Boundary Map" : "Show Boundary Map", ImVec2(totalWidth, 0.0f))) {
                // Show density map preview
                if (volumeMapPreview == nullptr) {
                    volumeMapPreview = new Texture3DPanel("Volume Map", boundaryField.texture, boundaryField.resolution.x, boundaryField.resolution.y, boundaryField.resolution.z);
                    manager->addPanel(volumeMapPreview);

                    volumeMapPreview->addOnCloseListener([this] () {
                        volumeMapPreview = nullptr;
                    });
                } else {
                    volumeMapPreview->close();
                    volumeMapPreview = nullptr;
                }
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Save Settings", ImVec2(totalWidth, 0))) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("SaveLocation", "Choose location", ".json", config);
    }

    if (ImGuiFileDialog::Instance()->Display("SaveLocation")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            saveSettings(filePathName);
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
            loadSettings(filePathName);
        }

        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::End();
}

void SimulationSettingsPanel::saveSettings(const std::string& path) const {
    Config config;

    // Particles
    config.set("h",                 simulation->settings.h,             "particles");
    config.set("p_0",               simulation->settings.p_0,           "particles");
    config.set("alpha",             simulation->settings.alpha,         "particles");
    config.set("speedOfSound",      simulation->settings.speedSound,    "particles");
    config.set("surfaceTension",    simulation->settings.surfaceTension,"particles");

    // Gravity
    config.set("enabled",           simulation->settings.enableGravity,  "gravity");
    config.set("force",             simulation->settings.gravity,        "gravity");

    // CFL
    config.set("enabled",           simulation->settings.useCFL,        "CFL");
    config.set("factor",            simulation->settings.safetyFactor,  "CFL");

    config.save(path);
}

void SimulationSettingsPanel::loadSettings(const std::string& path) const {
    Config config;

    if (config.load(path)) {
        // Particles
        simulation->settings.h              = config.get<float>("h",                simulation->settings.h,                 "particles");
        simulation->settings.p_0            = config.get<float>("p_0",              simulation->settings.p_0,               "particles");
        simulation->settings.alpha          = config.get<float>("alpha",            simulation->settings.alpha,             "particles");
        simulation->settings.speedSound     = config.get<float>("speedOfSound",     simulation->settings.speedSound,        "particles");
        simulation->settings.surfaceTension = config.get<float>("surfaceTension",   simulation->settings.surfaceTension,    "particles");

        simulation->settings.enableGravity  = config.get<bool> ("enabled",          simulation->settings.enableGravity,     "gravity");
        simulation->settings.gravity        = config.get<float>("force",            simulation->settings.gravity,           "gravity");

        simulation->settings.useCFL         = config.get<bool> ("enabled",          simulation->settings.useCFL,            "CFL");
        simulation->settings.safetyFactor   = config.get<float>("factor",           simulation->settings.safetyFactor,      "CFL");
    }
}
