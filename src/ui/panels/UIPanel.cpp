//
// Created by ruut on 3/14/26.
//

#include "ui/UIManager.h"
#include "ui/panels/UIPanel.h"

UIPanel::UIPanel(const std::string& name) : name(name) {}

UIPanel::~UIPanel() {
    notifyOnCloseListeners();
}

std::string UIPanel::getName() const { return name; }

void UIPanel::close() {
    if (manager) {
        shouldClose = true;
    } else {
        delete this;
    }
}

void UIPanel::addOnCloseListener(const std::function<void()>& listener) {
    onCloseListeners.push_back(listener);
}

void UIPanel::notifyOnCloseListeners() const {
    for (auto& listener : onCloseListeners)
        listener();
}
