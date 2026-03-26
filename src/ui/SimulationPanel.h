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
    explicit SimulationPanel(const std::string& name, GLuint texture);

    void update(double deltaTime) override;
    void draw() override;

    std::vector<PanelEvent> getEvents();

    glm::vec2 getSize() const;
    bool wasResized() const;
    void clearResizeFlag();

    int isMouseDown() const;
    glm::vec2 getMousePosition() const;
    float getScroll() const;
private:
    GLuint texture;

    glm::vec2 viewportSize = { 0, 0 };
    glm::vec2 mousePosition = { 0, 0 };
    float mouseScroll = 0.0f;
    bool resized = false;

    std::vector<PanelEvent> events;

    bool dragging = false;
};

#endif //FLUIDSIMSPH_VIEWPORTPANEL_H