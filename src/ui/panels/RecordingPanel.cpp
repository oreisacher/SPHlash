//
// Created by ruut on 10/23/26.
//

#ifdef HAS_FFMPEG

#include "ui/panels/RecordingPanel.h"

#include <format>

#include "imgui.h"
#include "core/Editor.h"
#include "utils/Logger.h"
#include <misc/cpp/imgui_stdlib.h>

RecordingPanel::RecordingPanel(const Editor* editor, Renderer* renderer) : UIPanel("Recording"), renderer(renderer) {
    simulation = editor->getScene()->getNode<SimulationNode>();
    buffer = renderer->getFramebuffer();
}

void RecordingPanel::update(double deltaTime) {
    if (!isRecording)
        return;

    if (simulation->getSimulationTime() < nextFrameTime)
        return;

    // Read pixel data from framebuffer
    glm::vec2 size = buffer->getSize();
    std::vector<uint8_t> pixels(size.x * size.y * 4);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, buffer->getID());
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glReadPixels(
        0, 0, size.x, size.y,
        GL_RGBA, GL_UNSIGNED_BYTE,
        pixels.data()
    );

    const size_t stride = static_cast<size_t>(size.x) * 4;

    for (int y = 0; y < size.y / 2; ++y) {
        uint8_t* top = pixels.data() + y * stride;
        uint8_t* bottom = pixels.data()
                        + ((int)size.y - 1 - y) * stride;

        std::swap_ranges(top, top + stride, bottom);
    }

    if (writer.writeFrame(pixels.data())) {
        frameCounter++;

        float runtime = frameCounter * 1.0f / framerate;
        LOG_INFO(std::format("Captured Frame #{} - Runtime: {:.3f} s", frameCounter, runtime));
        nextFrameTime += 1.0f / framerate;

        if (duration != 0.0f && runtime >= duration) {
            writer.close();
            isRecording = false;
            simulation->pause();
        }
    } else {
        writer.close();
        isRecording = false;
        simulation->pause();
    }
}

void RecordingPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str(), &visible);
    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    ImGui::Text("Filename:");
    ImGui::InputText("###filename", &filename);

    ImGui::Text("Framerate:");
    ImGui::DragFloat("###framerate", &framerate, 0.001, 1.0f, 1000);

    ImGui::Text("Duration:");
    ImGui::DragFloat("###duration", &duration, 0.001, 0.0f);

    ImGui::BeginDisabled(isRecording);

    if (ImGui::Button("Start Recording", ImVec2(totalWidth, 0))) {
        nextFrameTime = simulation->getSimulationTime();
        frameCounter = 0;

        glm::vec2 size = buffer->getSize();
        if (writer.open(filename, size.x, size.y, framerate))
            isRecording = true;
    }

    ImGui::EndDisabled();
    ImGui::BeginDisabled(!isRecording);

    if (ImGui::Button("Stop Recording", ImVec2(totalWidth, 0))) {
        writer.close();
        isRecording = false;
    }

    ImGui::EndDisabled();

    if (isRecording) {
        ImGui::Text("Recorded frames: %d", frameCounter);
        ImGui::Text("Recorded length: %fs", frameCounter * 1.0f / framerate);
    }

    ImGui::End();
}

#endif
