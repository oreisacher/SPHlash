//
// Created by ruut on 3/14/26.
//

#ifndef FLUIDSIMSPH_TESTPANEL_H
#define FLUIDSIMSPH_TESTPANEL_H

#include "UIPanel.h"

class TestPanel : public UIPanel {
public:
    explicit TestPanel(const std::string& name);

    void draw() override;
};

#endif //FLUIDSIMSPH_TESTPANEL_H