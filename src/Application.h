//
// Created by ruut on 3/27/26.
//

#ifndef FLUIDSIMSPH_APPLICATION_H
#define FLUIDSIMSPH_APPLICATION_H

#include "rendering/Editor.h"
#include "rendering/Renderer.h"
#include "rendering/FluidSurfaceMesher.h"
#include "rendering/Scene.h"
#include "rendering/Window.h"
#include "simulation/FluidSimulation.h"
#include "ui/UIManager.h"
#include "utils/CameraNode.h"

class Application {
public:
    Application();
    ~Application();

    void run() const;

private:
    Editor* editor;
    FluidSimulation* simulation;
    Renderer* renderer;

    UIManager* ui;
    Window* window;

    Scene scene;
    CameraNode* camera;
};

#endif //FLUIDSIMSPH_APPLICATION_H