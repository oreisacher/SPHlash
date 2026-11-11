//
// Created by ruut on 4/10/26.
//

#include "imgui.h"
#include "ui/panels/ControlPanel.h"
#include "rendering/Renderer.h"

ControlPanel::ControlPanel(Editor* editor, Renderer* renderer)
    : UIPanel("Controls"), editor(editor), renderer(renderer) {
    simulation = editor->getScene()->getNode<SimulationNode>();
}

void ControlPanel::update(double deltaTime) { }

void ControlPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str(), &visible);

    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    const SimulationState simulationState = simulation->getState();

    if (ImGui::Button("Init Simulation", ImVec2(totalWidth, 0)))
        editor->init();

    if (ImGui::IsKeyPressed(ImGuiKey_R) && simulationState == UNINITIALIZED)
        editor->init();

    ImGui::BeginDisabled(simulationState == UNINITIALIZED);

    if (ImGui::Button(simulationState == RUNNING ? "Stop Simulation" : "Start Simulation", ImVec2(totalWidth, 0)))
        simulationState == PAUSED ? simulation->start() : simulation->pause();

    if (ImGui::IsKeyPressed(ImGuiKey_Space) && simulationState != UNINITIALIZED)
        simulationState == PAUSED ? simulation->start() : simulation->pause();

    if (ImGui::Button("Reset Simulation", ImVec2(totalWidth, 0)))
        editor->reset();

    if (ImGui::IsKeyPressed(ImGuiKey_R) && simulationState != UNINITIALIZED)
        editor->reset();

    ImGui::EndDisabled();

    ImGui::End();
}
