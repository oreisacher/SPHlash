//
// Created by ruut on 3/14/26.
//

#ifndef FLUIDSIMSPH_VIEWPORTPANEL_H
#define FLUIDSIMSPH_VIEWPORTPANEL_H

#include "UIPanel.h"
#include "../rendering/FluidRenderer.h"

struct PanelEvent {
    enum Type { RESIZE, MOUSEMOVE, MOUSEDOWN, MOUSEUP, MOUSEHOLD, SCROLL } type;

    union {
        struct { glm::vec2 size; } resize;
        struct { glm::vec2 pos; bool dragging; glm::vec2 delta; } mouseMove;
        struct { int button; } mouse;
        struct { float delta; } scroll;
    };
};

class SimulationPanel : public UIPanel {
public:
    explicit SimulationPanel(GLuint textureId);

    void update(double deltaTime) override;
    void draw() override;

    std::vector<PanelEvent> getEvents();

private:
    GLuint texture;

    std::vector<PanelEvent> events;

    glm::vec2 viewportSize = { 0, 0 };
    glm::vec2 mousePosition = { 0, 0 };
    bool dragging = false;
};

#endif //FLUIDSIMSPH_VIEWPORTPANEL_H