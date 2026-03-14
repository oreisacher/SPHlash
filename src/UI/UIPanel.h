//
// Created by ruut on 3/14/26.
//

#ifndef FLUIDSIMSPH_UIPANEL_H
#define FLUIDSIMSPH_UIPANEL_H

#include <string>

class UIPanel {
public:
    UIPanel(const std::string& name);

    virtual ~UIPanel() = default;
    virtual void draw() = 0;

    std::string getName() const;

protected:
    bool visible = true;
    std::string name;
};

#endif //FLUIDSIMSPH_UIPANEL_H