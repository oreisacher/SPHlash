//
// Created by ruut on 5/25/26.
//

#ifndef SPHLASH_BOUNDARIESPANEL_H
#define SPHLASH_BOUNDARIESPANEL_H

#include "Texture3DPanel.h"
#include "UIPanel.h"
#include "rendering/Editor.h"

class BoundariesPanel : public UIPanel {
public:
    explicit BoundariesPanel(Editor* editor);

    void update(double deltaTime) override;
    void draw() override;

private:
    Editor* editor;
    BoundaryBuilder* boundaryBuilder;
    Texture3DPanel* volumeMapPreview = nullptr;
};

#endif //SPHLASH_BOUNDARIESPANEL_H
