//
// Created by ruut on 4/12/26.
//

#ifndef FLUIDSIMSPH_RENDERINGSETTINGSPANEL_H
#define FLUIDSIMSPH_RENDERINGSETTINGSPANEL_H

#include "UIPanel.h"
#include "rendering/FluidRenderer.h"
#include "rendering/FluidSurfaceMesher.h"

class RenderingSettingsPanel : public UIPanel {
public:
    explicit RenderingSettingsPanel(FluidRenderer* renderer, FluidSurfaceMesher* surfaceMesher);

    void update(double deltaTime) override;
    void draw() override;

private:
    FluidRenderer* renderer;
    FluidSurfaceMesher* surfaceMesher;

    static inline const char* renderModes[] {
        "Particles - Solid Color",
        "Particles - Velocity",
        "Particles - Pressure",
        "Particles - Surface",
        "Marching Cubes Surface"
    };
};

#endif //FLUIDSIMSPH_RENDERINGSETTINGSPANEL_H
