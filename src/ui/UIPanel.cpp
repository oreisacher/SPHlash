//
// Created by ruut on 3/14/26.
//

#include "ui/UIPanel.h"

UIPanel::UIPanel(const std::string& name) : name(name) {}

std::string UIPanel::getName() const { return name; }
