//
// Created by ruut on 10/23/26.
//

#ifndef SPHLASH_RECORDINGPANEL_H
#define SPHLASH_RECORDINGPANEL_H

#include "UIPanel.h"
#include "core/Editor.h"
#include "rendering/Renderer.h"
#include "utils/VideoWriter.h"

class RecordingPanel : public UIPanel {
public:
    explicit RecordingPanel(const Editor* editor, Renderer* renderer);

    void update(double deltaTime) override;
    void draw() override;
private:
    Renderer* renderer;
    SimulationNode* simulation;

    VideoWriter writer;
    Framebuffer* buffer;

    bool isRecording = false;
    int frameCounter = 0;

    std::string filename = "recording.mp4";
    float framerate = 60;
    float nextFrameTime = 0.0f;
};

#endif //SPHLASH_RECORDINGPANEL_H
