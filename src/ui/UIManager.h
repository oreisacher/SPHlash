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
    UIManager(GLFWwindow* window);
    ~UIManager();

    void addPanel(UIPanel *panel);
    void removePanel(UIPanel *panel);
    void removePanel(const std::string& name);

    void update(double deltaTime) const;
    void draw() const;

    template<typename T>
    T* getPanel() {
        for (auto panel : panels) {
            if (auto correct = dynamic_cast<T*>(panel)) {
                return correct;
            }
        }

        return nullptr;
    }

private:
    std::vector<UIPanel*> panels;
    bool dockLayoutInitialized = false;
};

#endif //FLUIDSIMSPH_UIMANAGER_H