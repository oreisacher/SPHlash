//
// Created by ruut on 3/24/26.
//

#ifndef FLUIDSIMSPH_PARTICLESPANEL_H
#define FLUIDSIMSPH_PARTICLESPANEL_H

#include <cmath>
#include "UIPanel.h"
#include "simulation/FluidSimulation.h"

class Shader;
class Renderer;
class FluidSimulation;

class ParticlesPanel : public UIPanel {
public:
    explicit ParticlesPanel(FluidSimulation* simulation, ParticlesNode* particles);

    void update(double deltaTime) override;
    void draw() override;

private:
    FluidSimulation* simulation;
    Shader* particlesShader;

    ParticleHost particle;

    double time = 0.0f;

    bool showParticles = false;
    int selectedParticle = 0;

#ifdef METRIC_DENSITY
    // Average Density
    std::vector<float> averageDensity;
    float minAvgDensity = INFINITY;
    float maxAvgDensity = -INFINITY;

    // Average Density Error
    std::vector<float> averageDensityError;
    float minDensityError = INFINITY;
    float maxDensityError = -INFINITY;

    // Density Standard Deviation
    std::vector<float> densityStandardDeviation;
    float minDensityStandardDeviation = INFINITY;
    float maxDensityStandardDeviation = -INFINITY;
#endif

#ifdef METRIC_AVG_HEIGHT
    // Average Top Surface Height
    std::vector<float> averageTopSurfaceHeigth;
#endif
};

#endif //FLUIDSIMSPH_PARTICLESPANEL_H