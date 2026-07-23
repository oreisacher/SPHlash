//
// Created by ruut on 4/10/26.
//

#include "../../../include/ui/panels/ControlPanel.h"
#include "imgui.h"
#include "../../../include/rendering/Renderer.h"

ControlPanel::ControlPanel(Editor* editor, FluidSimulation* simulation, Renderer* renderer)
    : UIPanel("Controls"), editor(editor), simulation(simulation), renderer(renderer) {
}

void ControlPanel::preDraw(double deltaTime) { }

void ControlPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());

    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    if (ImGui::Button("Init Simulation", ImVec2(totalWidth, 0))) {
        // Build boundary map
        if (editor->getBoundaryBuilder()->settings.buildOnInit)
            editor->buildBoundaryDensityMap();

        // Init simulation
        simulation->init();
    }

    const SimulationState simulationState = simulation->getState();

    ImGui::BeginDisabled(simulationState == UNINITIALIZED);

    if (ImGui::Button(simulationState == RUNNING ? "Stop Simulation" : "Start Simulation", ImVec2(totalWidth, 0)))
        simulationState == PAUSED ? simulation->start() : simulation->pause();

    if (ImGui::IsKeyPressed(ImGuiKey_Space) && simulationState != UNINITIALIZED)
        simulationState == PAUSED ? simulation->start() : simulation->pause();

    if (ImGui::Button("Reset Simulation", ImVec2(totalWidth, 0)))
        simulation->reset();

    if (ImGui::IsKeyPressed(ImGuiKey_R) && simulationState != UNINITIALIZED)
        simulation->reset();

    ImGui::EndDisabled();

    ImGui::End();
}
