//
// Created by ruut on 4/10/26.
//

#ifndef FLUIDSIMSPH_CONTROLPANEL_H
#define FLUIDSIMSPH_CONTROLPANEL_H

#include "UIPanel.h"
#include "core/Editor.h"
#include "rendering/FluidSurfaceMesher.h"
#include "simulation/SimulationNode.h"

class ControlPanel : public UIPanel {
public:
    explicit ControlPanel(Editor* editor, Renderer* renderer);

    void update(double deltaTime) override;
    void draw() override;

private:
    Editor* editor;
    SimulationNode* simulation;
    Renderer* renderer;

    float timeStep = 0.0;
};

#endif //FLUIDSIMSPH_CONTROLPANEL_H
