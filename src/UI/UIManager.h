//
// Created by ruut on 3/14/26.
//

#ifndef FLUIDSIMSPH_UIMANAGER_H
#define FLUIDSIMSPH_UIMANAGER_H
#include <vector>

#include "UIPanel.h"
#include "GLFW/glfw3.h"

class UIManager {
public:
    ~UIManager();

    void init(GLFWwindow* window);

    void addPanel(UIPanel *panel);
    void removePanel(UIPanel *panel);
    void removePanel(const std::string& name);

    void draw() const;

private:
    std::pmr::vector<UIPanel*> panels;
};

#endif //FLUIDSIMSPH_UIMANAGER_H