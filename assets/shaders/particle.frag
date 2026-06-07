#version 430 core

#define PI 3.1415

flat in vec3 Velocity;
flat in vec4 Properties;
flat in vec4 Flags;
flat in vec4 Color;

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
        FragColor = Color;
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
        float t = clamp(Flags.x / maxSurfaceTension, 0.0, 1.0);
        FragColor = mix(Color, vec4(1.0), t);
        return;
    }
}