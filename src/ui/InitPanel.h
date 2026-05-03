//
// Created by ruut on 4/10/26.
//

#ifndef FLUIDSIMSPH_INITPANEL_H
#define FLUIDSIMSPH_INITPANEL_H

#include "UIPanel.h"
#include "rendering/Editor.h"
#include "rendering/FluidRenderer.h"
#include "rendering/FluidSurfaceMesher.h"
#include "simulation/FluidSimulation.h"

class InitPanel : public UIPanel{
public:
    explicit InitPanel(Editor* editor, FluidSurfaceMesher* mesher);

    void update(double deltaTime) override;
    void draw() override;

private:
    FluidSimulation* simulation;
    FluidRenderer* renderer;
    FluidSurfaceMesher* mesher;
    Editor* editor;

    int particlesToSpawn = 0;
    float spawnVolume = 0;
    float massPerParticle = 0;

    void recalculateParticles();
};

#endif //FLUIDSIMSPH_INITPANEL_H
