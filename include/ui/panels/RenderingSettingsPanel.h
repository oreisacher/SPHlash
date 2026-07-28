//
// Created by ruut on 4/12/26.
//

#ifndef FLUIDSIMSPH_RENDERINGSETTINGSPANEL_H
#define FLUIDSIMSPH_RENDERINGSETTINGSPANEL_H

#include "UIPanel.h"
#include "../../rendering/Renderer.h"
#include "../../rendering/FluidSurfaceMesher.h"
#include "core/Editor.h"

class RenderingSettingsPanel : public UIPanel {
public:
    explicit RenderingSettingsPanel(const Editor* editor);

    void update(double deltaTime) override;
    void draw() override;

private:
    FluidSurfaceMesher* surfaceMesher;
    FluidSurfaceScreenSpace* surfaceScreenSpace;

    ParticlesNode* particles;

    static inline const char* renderModes[] {
        "Particles - Solid Color",
        "Particles - Velocity",
        "Particles - Pressure",
        "Particles - Surface",
        "Particles - Top Surface",
        "Marching Cubes Surface",
        "Screen Space Surface"
    };
};

#endif //FLUIDSIMSPH_RENDERINGSETTINGSPANEL_H
