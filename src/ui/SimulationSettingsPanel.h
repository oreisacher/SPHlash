//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_SETTINGSPANEL_H
#define FLUIDSIMSPH_SETTINGSPANEL_H

#include "Texture3DPanel.h"
#include "UIPanel.h"
#include "rendering/Editor.h"
#include "rendering/Renderer.h"

class SimulationSettingsPanel : public UIPanel {
public:
    explicit SimulationSettingsPanel(Editor* editor, FluidSimulation* simulation, Renderer* renderer);

    void update(double deltaTime) override;
    void draw() override;

private:
    Editor* editor;
    FluidSimulation* simulation;
    Renderer* renderer;
    BoundaryBuilder* boundaryBuilder;

    Texture3DPanel* volumeMapPreview = nullptr;

    void saveSettings(const std::string& path) const;
    void loadSettings(const std::string& path) const;
};

#endif //FLUIDSIMSPH_SETTINGSPANEL_H