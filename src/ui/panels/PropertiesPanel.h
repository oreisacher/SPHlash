//
// Created by ruut on 5/31/26.
//

#ifndef SPHLASH_PROPERTIESPANEL_H
#define SPHLASH_PROPERTIESPANEL_H

#include "simulation/scene/ISelectable.h"
#include "UIPanel.h"
#include "core/Editor.h"

class PropertiesPanel : public UIPanel {
public:
    PropertiesPanel(Editor* editor);

    void preDraw(double deltaTime) override;
    void draw() override;

private:
    Editor* editor;
    ISelectable* selected;
    Properties properties;
};

#endif //SPHLASH_PROPERTIESPANEL_H
