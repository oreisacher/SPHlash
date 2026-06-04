//
// Created by ruut on 4/10/26.
//

#include "ControlPanel.h"
#include "imgui.h"
#include <iostream>

ControlPanel::ControlPanel(Editor* editor, FluidSimulation* simulation, FluidRenderer* renderer, FluidSurfaceMesher* mesher)
    : UIPanel("Controls"), editor(editor), simulation(simulation), renderer(renderer), mesher(mesher) {
}

void ControlPanel::update(double deltaTime) { }

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

        // Setup renderer
        renderer->updateParticleData();
        mesher->generateMesh(true);
    }

    const SimulationState simulationState = simulation->getState();

    ImGui::BeginDisabled(simulationState == UNINITIALIZED);

    if (ImGui::Button(simulationState == RUNNING ? "Stop Simulation" : "Start Simulation", ImVec2(totalWidth, 0)))
        simulationState == PAUSED ? simulation->start() : simulation->pause();

    if (ImGui::IsKeyPressed(ImGuiKey_Space) && simulationState != UNINITIALIZED)
        simulationState == PAUSED ? simulation->start() : simulation->pause();

    if (ImGui::Button("Reset Simulation", ImVec2(totalWidth, 0))) {
        simulation->reset();
        mesher->generateMesh(true);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_R) && simulationState != UNINITIALIZED) {
        simulation->reset();
        mesher->generateMesh(true);
    }

    ImGui::Separator();

    ImGui::Text("Manual Frame Step Size (ms):");
    ImGui::DragFloat("##fss", &stepSize, 1, 1000, 1.0);
    if (ImGui::Button("Step Single Frame", ImVec2(totalWidth, 0)))
        simulation->step(stepSize / 1000.0f);

    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && simulation->getState() != UNINITIALIZED)
        simulation->step(stepSize / 1000.0f);


    ImGui::EndDisabled();

    ImGui::End();
}
