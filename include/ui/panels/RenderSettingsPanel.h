//
// Created by ruut on 10/24/26.
//

#ifndef SPHLASH_RENDERSETTINGSPANEL_H
#define SPHLASH_RENDERSETTINGSPANEL_H

#include "UIPanel.h"
#include "core/Editor.h"
#include "rendering/Renderer.h"

class RenderSettingsPanel : public UIPanel {
public:
    explicit RenderSettingsPanel(Editor* editor, Renderer* renderer);

    void update(double deltaTime) override;
    void draw() override;
private:
    Renderer* renderer;
    Editor* editor;
};

#endif //SPHLASH_RENDERSETTINGSPANEL_H
