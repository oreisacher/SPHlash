//
// Created by ruut on 3/16/26.
//

#include "ui/panels/SimulationSettingsPanel.h"
#include "simulation/SimulationNode.h"
#include "ImGuiFileDialog.h"
#include "ui/UIManager.h"
#include "glm/gtc/type_ptr.inl"
#include "imgui.h"
#include "core/Editor.h"
#include "rendering/Renderer.h"

SimulationSettingsPanel::SimulationSettingsPanel(Editor* editor, Renderer* renderer)
    : UIPanel("Simulation Settings"), editor(editor), renderer(renderer)
{
    simulation = editor->getScene()->getNode<SimulationNode>();
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
        ImGui::DragFloat("##H", &simulation->settings.h, 0.0001, 0.0001, 1.0, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Text("P_0:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Rest Density");
        ImGui::DragFloat("##P_0", &simulation->settings.p_0, 1.0f, 1.0f, 10000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Text("Max Velocity:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Maximum velocity to expect. Estimate like sqrt(2gH) for example for dam break");
        ImGui::DragFloat("##mv", &simulation->settings.maxVelocity, 0.001f, 0.0f, 1024.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Text("Alpha:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Viscosity Coefficient");
        ImGui::DragFloat("##alpha", &simulation->settings.alpha, 0.001f, 0.0f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Text("Surface Tension:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Surface Tension Strength");
        ImGui::DragFloat("##T", &simulation->settings.surfaceTension, 0.001f, 0.0f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }

    if (ImGui::CollapsingHeader("Kernels")) {
        ImGui::Text("Density Kernel:");
        int currentDensityKernel = static_cast<int>(simulation->settings.kernelSettings.density);
        if (ImGui::Combo("##DK", &currentDensityKernel, kernelNames, IM_ARRAYSIZE(kernelNames))) {
            simulation->settings.kernelSettings.density = static_cast<KernelType>(currentDensityKernel);
        }

        ImGui::Spacing();

        ImGui::Text("Pressure Kernel:");
        int currentPressureKernel = static_cast<int>(simulation->settings.kernelSettings.pressure);
        if (ImGui::Combo("##PK", &currentPressureKernel, kernelNames, IM_ARRAYSIZE(kernelNames))) {
            simulation->settings.kernelSettings.pressure = static_cast<KernelType>(currentPressureKernel);
        }

        ImGui::Spacing();

        ImGui::Text("Viscosity Kernel:");
        int currentViscosityKernel = static_cast<int>(simulation->settings.kernelSettings.viscosity);
        if (ImGui::Combo("##VK", &currentViscosityKernel, kernelNames, IM_ARRAYSIZE(kernelNames))) {
            simulation->settings.kernelSettings.viscosity = static_cast<KernelType>(currentViscosityKernel);
        }

        ImGui::Spacing();

        ImGui::Text("Surface Tension Kernel:");
        int currentSurfaceTensionKernel = static_cast<int>(simulation->settings.kernelSettings.surfaceTension);
        if (ImGui::Combo("##STK", &currentSurfaceTensionKernel, kernelNames, IM_ARRAYSIZE(kernelNames))) {
            simulation->settings.kernelSettings.surfaceTension = static_cast<KernelType>(currentSurfaceTensionKernel);
        }

        ImGui::Spacing();
    }

    if (ImGui::CollapsingHeader("Gravity Settings")) {
        ImGui::Checkbox("Enable##g", &simulation->settings.enableGravity);

        ImGui::Text("Gravity:");
        ImGui::DragFloat("##g", &simulation->settings.gravity, 0.01, -1024.f, 1024.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }

    if (ImGui::CollapsingHeader("Simulation Settings")) {
        ImGui::Text("Simulation Steps per Frame:");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("How many steps the simulation should move forward each frame");
        ImGui::DragInt("##sspf", &simulation->settings.simulationSteps, 0.1, 1, 128, "%d", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Spacing();

        ImGui::Checkbox("Enable fixed Time Step##fixedTimeStep", &simulation->settings.useFixedTimeStep);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("If disabled, the actual frame time is used");

        ImGui::Text("Time Step (ms):");
        ImGui::DragFloat("##fts", &simulation->settings.fixedTimeStep, 0.0001, 0.0f, 1000.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }

    if (ImGui::CollapsingHeader("CFL Settings")) {
        ImGui::Checkbox("Enable CFL##cfl", &simulation->settings.useCFL);

        ImGui::Text("Safety Factor:");
        ImGui::DragFloat("##cflSF", &simulation->settings.safetyFactor, 0.001, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    }

    if (ImGui::CollapsingHeader("Boundary Settings")) {
        ImGui::Checkbox("Build on Init##bbInitBuild", &boundaryBuilder->settings.buildOnInit);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Build volume map on simulation init. Must be rebuild after changing H!");

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
        ImGuiFileDialog::Instance()->OpenDialog("SaveLocation", "Choose location", ".para", config);
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
        ImGuiFileDialog::Instance()->OpenDialog("LoadLocation", "Choose location", ".para", config);
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
