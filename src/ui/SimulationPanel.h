//
// Created by ruut on 3/14/26.
//

#ifndef FLUIDSIMSPH_VIEWPORTPANEL_H
#define FLUIDSIMSPH_VIEWPORTPANEL_H

#include "UIPanel.h"
#include "../rendering/FluidRenderer.h"

class SimulationPanel : public UIPanel {
public:
    explicit SimulationPanel(const std::string& name, GLuint texture);

    void update(double deltaTime) override;
    void draw() override;

    glm::vec2 getSize() const;
    bool wasResized() const;
    void clearResizeFlag();

    int isMouseDown() const;
    glm::vec2 getMousePosition() const;
    float getScroll() const;
private:
    GLuint texture;

    glm::vec2 viewportSize = { 0, 0 };
    bool resized = false;

    glm::vec2 mousePosition = { 0, 0 };
    float mouseScroll = 0.0f;
};

#endif //FLUIDSIMSPH_VIEWPORTPANEL_H