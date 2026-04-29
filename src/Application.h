//
// Created by ruut on 3/27/26.
//

#ifndef FLUIDSIMSPH_APPLICATION_H
#define FLUIDSIMSPH_APPLICATION_H

#include "rendering/Editor.h"
#include "rendering/FluidRenderer.h"
#include "rendering/FluidSurfaceMesher.h"
#include "rendering/Scene.h"
#include "rendering/Window.h"
#include "simulation/FluidSimulationGPU.h"
#include "ui/UIManager.h"
#include "utils/Camera.h"

class Application {
public:
    Application();
    ~Application();

    void run() const;
    void processEvents() const;

private:
    Editor* editor;
    FluidSimulationGPU* simulation;
    FluidRenderer* renderer;

    FluidSurfaceMesher* surfaceMesher;

    UIManager* ui;
    Window* window;

    Scene scene;
};

#endif //FLUIDSIMSPH_APPLICATION_H