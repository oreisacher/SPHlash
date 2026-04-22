//
// Created by ruut on 4/13/26.
//

#ifndef SPHLASH_MODELSPANEL_H
#define SPHLASH_MODELSPANEL_H

#include "UIPanel.h"
#include "rendering/FluidRenderer.h"

class BoundariesPanel : public UIPanel {
public:
    explicit BoundariesPanel();

    void update(double deltaTime) override;
    void draw() override;

private:
    FluidRenderer* renderer;

    int selected = -1;
    Model* selectedModel = nullptr;
    Transform* selectedTransform = nullptr;
};

#endif //SPHLASH_MODELSPANEL_H
