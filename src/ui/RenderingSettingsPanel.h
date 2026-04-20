//
// Created by ruut on 4/12/26.
//

#ifndef FLUIDSIMSPH_RENDERINGSETTINGSPANEL_H
#define FLUIDSIMSPH_RENDERINGSETTINGSPANEL_H

#include "UIPanel.h"
#include "rendering/FluidRenderer.h"

class RenderingSettingsPanel : public UIPanel {
public:
    explicit RenderingSettingsPanel();

    void update(double deltaTime) override;
    void draw() override;

private:
    FluidRenderer* renderer;

    static inline const char* renderModes[] {
        "Solid Color",
        "Velocity",
        "Pressure"
    };
};

#endif //FLUIDSIMSPH_RENDERINGSETTINGSPANEL_H
