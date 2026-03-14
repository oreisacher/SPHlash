//
// Created by ruut on 3/14/26.
//

#ifndef FLUIDSIMSPH_VIEWPORTPANEL_H
#define FLUIDSIMSPH_VIEWPORTPANEL_H

#include "UIPanel.h"
#include "../Rendering/FluidRenderer.h"

class SimulationPanel : public UIPanel {
public:
    explicit SimulationPanel(const std::string& name, GLuint texture);

    void draw() override;

    glm::vec2 getSize() const;
    bool wasResized() const;
    void clearResizeFlag();

    int isMouseClicked() const;
    glm::vec2 getMousePosition() const;
private:
    GLuint texture;

    glm::vec2 viewportSize = { 0, 0 };
    bool resized = false;
};

#endif //FLUIDSIMSPH_VIEWPORTPANEL_H