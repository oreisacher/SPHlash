//
// Created by ruut on 3/16/26.
//

#include "FPSPanel.h"

#include <format>
#include "imgui.h"

FPSPanel::FPSPanel(const std::string& name) : UIPanel(name) { }

void FPSPanel::update(double deltaTime) {
    frameTime = deltaTime;
    fpsTimer += deltaTime;
    frames++;

    if (fpsTimer >= 1.0f) {
        fps = frames / fpsTimer;
        frames = 0;
        fpsTimer = 0.0f;
    }
}

void FPSPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());

    ImGui::Text("FPS: %d", fps);
    ImGui::Text("Frame Time: %.3f ms", frameTime);

    ImGui::End();
}
