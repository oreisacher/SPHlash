//
// Created by ruut on 3/24/26.
//

#ifndef FLUIDSIMSPH_PARTICLESPANEL_H
#define FLUIDSIMSPH_PARTICLESPANEL_H

#include "UIPanel.h"
#include "glad/gl.h"
#include "rendering/Shader.h"

class ParticlesPanel : public UIPanel {
public:
    explicit ParticlesPanel(const std::string& name, GLuint particlesVBO, int particleCount);

    void update(double deltaTime) override;
    void draw() override;

    void setShader(Shader* shader);

private:
    GLuint particlesVBO;
    int particleCount;
    bool showParticles = false;
    int selectedParticle = 0;

    Shader* shader;
};

#endif //FLUIDSIMSPH_PARTICLESPANEL_H