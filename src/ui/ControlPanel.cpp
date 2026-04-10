//
// Created by ruut on 4/10/26.
//

#include "ControlPanel.h"
#include "imgui.h"
#include <iostream>

ControlPanel::ControlPanel(const std::string& name, FluidSimulationGPU* simulation)
    : UIPanel(name), simulation(simulation)
{ }

void ControlPanel::update(double deltaTime) { }

void ControlPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());

    ImGui::BeginDisabled(simulation->state == UNINITIALIZED);
    const char* startStopText = simulation->state == RUNNING ? "Stop Simulation" : "Start Simulation";
    if (ImGui::Button(startStopText)) {
        simulation->state = simulation->state == PAUSED ? RUNNING : PAUSED;
    }
    ImGui::EndDisabled();

    ImGui::End();
}
