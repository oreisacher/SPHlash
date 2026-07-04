//
// Created by ruut on 3/27/26.
//

#ifndef FLUIDSIMSPH_APPLICATION_H
#define FLUIDSIMSPH_APPLICATION_H

#include "Editor.h"
#include "rendering/Renderer.h"
#include "simulation/scene/Scene.h"
#include "simulation/FluidSimulation.h"
#include "ui/UIManager.h"
#include "simulation/scene/nodes/CameraNode.h"

class Application {
public:
    Application();
    ~Application();

    void run() const;

private:
    Editor* _editor                 = nullptr;
    FluidSimulation* _simulation    = nullptr;
    Renderer* _renderer             = nullptr;

    UIManager* _ui                  = nullptr;
    Window* _window                 = nullptr;

    Scene* _scene                   = nullptr;
    CameraNode* _camera             = nullptr;
};

#endif //FLUIDSIMSPH_APPLICATION_H