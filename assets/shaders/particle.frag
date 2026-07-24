#version 430 core

#define PI 3.1415

flat in vec3 Velocity;
flat in vec4 Properties;
flat in vec4 Flags;
flat in vec3 Color;

uniform int selectedParticleID = -1;

// --- Rendering modes ---
uniform int mode = 0;

uniform vec3 minVelColor;
uniform float minVel;
uniform vec3 maxVelColor;
uniform float maxVel;

uniform vec3 minPresColor;
uniform float minPres;
uniform vec3 maxPresColor;
uniform float maxPres;

uniform float maxSurfaceTension;

uniform vec3 solidColor;

out vec4 FragColor;

void main() {
    if (selectedParticleID == Properties.w) {
        FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
        return;
    }

    // SOLID COLOR PARTICLES
    if (mode == 0) {
        FragColor = vec4(Color, 1.0);
        return;
    }

    // VELOCITY PARTICLES
    if (mode == 1) {
        float speed = length(Velocity);
        float t = (speed - minVel) / (maxVel - minVel + 1e-6f);
        FragColor = vec4(mix(minVelColor, maxVelColor, t), 1.0);
        return;
    }

    // PRESSURE PARTICLES
    if (mode == 2) {
        float t = (Properties.z - minPres) / (maxPres - minPres);
        FragColor = vec4(mix(minPresColor, maxPresColor, t), 1.0);
        return;
    }

    // SURFACE PARTICLES
    if (mode == 3) {
        if (Flags.x > maxSurfaceTension) {
            FragColor = vec4(1.0);
            return;
        }

        FragColor = vec4(Color, 1.0);
        return;
    }

    // TOP SURFACE PARTICLES
    if (mode == 4) {
        if (Flags.y > 0.5) {
            FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
            return;
        }

        FragColor = vec4(Color, 1.0);
        return;
    }
}