//
// Created by ruut on 3/27/26.
//

#ifndef FLUIDSIMSPH_APPLICATION_H
#define FLUIDSIMSPH_APPLICATION_H

#include "rendering/FluidRenderer.h"
#include "rendering/Window.h"
#include "simulation/FluidSimulationGPU.h"
#include "ui/UIManager.h"
#include "utils/Camera.h"

class Application {
public:
    Application();
    ~Application();

    void run() const;

private:
    FluidSimulationGPU* simulation;
    FluidRenderer* renderer;

    UIManager* ui;
    Window* window;

    Shader* shader;
    Camera* camera;
};

#endif //FLUIDSIMSPH_APPLICATION_H