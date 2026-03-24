//
// Created by ruut on 3/24/26.
//

#ifndef FLUIDSIMSPH_PARTICLESPANEL_H
#define FLUIDSIMSPH_PARTICLESPANEL_H

#include "UIPanel.h"
#include "glad/gl.h"

class ParticlesPanel : public UIPanel {
public:
    explicit ParticlesPanel(const std::string& name, GLuint particlesVBO, int particleCount);

    void update(double deltaTime) override;
    void draw() override;

private:
    GLuint particlesVBO;
    int particleCount;
    bool showParticles = false;
    int selectedParticle = 0;
};

#endif //FLUIDSIMSPH_PARTICLESPANEL_H