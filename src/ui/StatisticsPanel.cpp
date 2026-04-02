//
// Created by ruut on 3/16/26.
//

#include "StatisticsPanel.h"

#include <format>
#include "imgui.h"

StatisticsPanel::StatisticsPanel(const std::string& name, FluidSimulationGPU* simulation)
    : UIPanel(name), simulation(simulation) { }

void StatisticsPanel::update(double deltaTime) {
    // Update fps
    frameTime = deltaTime;
    fpsTimer += deltaTime;
    frames++;

    if (fpsTimer >= UPDATE_TIME) {
        fps = frames / fpsTimer;
        frames = 0;
        fpsTimer = 0.0f;

        // Update fps graph
        fpsTimes[fpsTimesOffset] = fps;
        fpsTimesOffset = (fpsTimesOffset + 1) % FPS_TIMES_COUNT;

        // Update cfl
        cflTimes[cflTimesOffset] = simulation->cfl;
        cflTimesOffset = (cflTimesOffset + 1) % CFL_COUNT;
    }
}

void StatisticsPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());

    ImGui::Text("FPS: %d", fps);
    ImGui::Text("Frame Time: %.3f ms", frameTime * 1000.0f);

    ImGui::PlotLines(
        "FPS History",
        fpsTimes,
        FPS_TIMES_COUNT,
        fpsTimesOffset,
        nullptr,
        0,
        FLT_MAX,
        ImVec2(0, 50)
    );

    ImGui::Separator();

    ImGui::Text("CFL: %f ms", cflTimes[cflTimesOffset - 1] * 1000.0f);

    ImGui::PlotLines(
        "CFL History",
        cflTimes,
        CFL_COUNT,
        cflTimesOffset,
        nullptr,
        0,
        FLT_MAX,
        ImVec2(0, 50)
    );

    ImGui::End();
}
