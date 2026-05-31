//
// Created by ruut on 4/13/26.
//

#ifndef SPHLASH_MODELSPANEL_H
#define SPHLASH_MODELSPANEL_H

#include "UIPanel.h"
#include "rendering/Editor.h"
#include "rendering/FluidRenderer.h"

class ScenePanel : public UIPanel {
public:
    explicit ScenePanel(Scene& scene, Editor* editor, FluidRenderer* renderer);

    void update(double deltaTime) override;
    void draw() override;

private:
    Editor* editor;
    FluidRenderer* renderer;
    Scene* scene;

    int selected = -1;
};

#endif //SPHLASH_MODELSPANEL_H
