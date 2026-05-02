//
// Created by ruut on 4/10/26.
//

#include "ControlPanel.h"
#include "imgui.h"
#include <iostream>

ControlPanel::ControlPanel()
    : UIPanel("Controls") {
    simulation = FluidSimulation::getInstance();
}

void ControlPanel::update(double deltaTime) { }

void ControlPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());

    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    ImGui::BeginDisabled(simulation->state == UNINITIALIZED);

    if (ImGui::Button(simulation->state == RUNNING ? "Stop Simulation" : "Start Simulation", ImVec2(totalWidth, 0)))
        simulation->state == PAUSED ? simulation->start() : simulation->pause();

    if (ImGui::IsKeyPressed(ImGuiKey_Space) && simulation->state != UNINITIALIZED)
        simulation->state == PAUSED ? simulation->start() : simulation->pause();

    if (ImGui::Button("Reset Simulation", ImVec2(totalWidth, 0)))
        simulation->reset();

    if (ImGui::IsKeyPressed(ImGuiKey_R) && simulation->state != UNINITIALIZED)
        simulation->reset();

    ImGui::Separator();

    ImGui::Text("Manual Frame Step Size (ms):");
    ImGui::DragFloat("##fss", &stepSize, 1, 1000, 1.0);
    if (ImGui::Button("Step Single Frame", ImVec2(totalWidth, 0)))
        simulation->step(stepSize / 1000.0f);

    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && simulation->state != UNINITIALIZED)
        simulation->step(stepSize / 1000.0f);


    ImGui::EndDisabled();

    ImGui::End();
}
