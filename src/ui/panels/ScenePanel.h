//
// Created by ruut on 4/13/26.
//

#ifndef SPHLASH_MODELSPANEL_H
#define SPHLASH_MODELSPANEL_H

#include "UIPanel.h"
#include "core/Editor.h"
#include "rendering/Renderer.h"

class ScenePanel : public UIPanel {
public:
    explicit ScenePanel(Editor* editor, Renderer* renderer);

    void preDraw(double deltaTime) override;
    void draw() override;

private:
    Editor* editor;
    Renderer* renderer;
};

#endif //SPHLASH_MODELSPANEL_H
