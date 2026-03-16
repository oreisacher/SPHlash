//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_FPSPANEL_H
#define FLUIDSIMSPH_FPSPANEL_H

#include "UIPanel.h"

class FPSPanel : public UIPanel {
public:
    explicit FPSPanel(const std::string& name);

    void update(double deltaTime) override;
    void draw() override;

private:
    double fpsTimer = 0.0;
    int frames = 0;

    // Variables for draw call
    int fps = 0;
    double frameTime = 0.0f;
};

#endif //FLUIDSIMSPH_FPSPANEL_H