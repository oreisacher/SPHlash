//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_STATISTICSPANEL_H
#define FLUIDSIMSPH_STATISTICSPANEL_H

#include "UIPanel.h"
#include "simulation/FluidSimulation.h"

#define UPDATE_TIME 0.2f
#define FPS_TIMES_COUNT 100
#define CFL_COUNT 100

class StatisticsPanel : public UIPanel {
public:
    explicit StatisticsPanel();

    void update(double deltaTime) override;
    void draw() override;

private:
    FluidSimulation* simulation;

    double fpsTimer = 0.0;
    int frames = 0;

    // Variables for draw call
    int fps = 0;
    double frameTime = 0.0f;

    // FPS
    float fpsTimes[FPS_TIMES_COUNT] = {};
    int fpsTimesOffset = 0;

    // CFL
    float cflTimes[CFL_COUNT] = {};
    int cflTimesOffset = 0;
};

#endif //FLUIDSIMSPH_STATISTICSPANEL_H