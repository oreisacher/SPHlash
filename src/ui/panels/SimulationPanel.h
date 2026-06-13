//
// Created by ruut on 3/14/26.
//

#ifndef FLUIDSIMSPH_VIEWPORTPANEL_H
#define FLUIDSIMSPH_VIEWPORTPANEL_H

#include "UIPanel.h"
#include "rendering/Renderer.h"
#include "core/Editor.h"

class SimulationPanel : public UIPanel {
public:
    explicit SimulationPanel(Editor* editor, Renderer* renderer);

    void preDraw(double deltaTime) override;
    void draw() override;
    void postDraw(double deltaTime) override;

    static inline CameraNode* camera = nullptr;

private:
    Editor* editor;
    Renderer* renderer;

    bool pendingResize = false;

    glm::vec2 viewportSize = { 0, 0 };
    glm::vec2 mousePosition = { 0, 0 };
    bool dragging = false;
};

#endif //FLUIDSIMSPH_VIEWPORTPANEL_H