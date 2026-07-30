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

    void update(double deltaTime) override;
    void draw() override;
    void drawMenu() override;

private:
    Editor* editor;
    Renderer* renderer;

    int _selected_scene = 0;
    std::vector<std::string> _scene_list{};
    void refreshSceneList();
};

#endif //SPHLASH_MODELSPANEL_H
