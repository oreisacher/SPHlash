#version 430 core

#define PI 3.1415

flat in vec3 Velocity;
flat in vec4 Properties;
flat in vec4 Flags;
flat in vec4 Color;

uniform int selectedParticleID = -1;

// --- Rendering modes ---
uniform int mode = 0;

uniform vec4 minVelColor;
uniform float minVel;
uniform vec4 maxVelColor;
uniform float maxVel;

uniform vec4 minPresColor;
uniform float minPres;
uniform vec4 maxPresColor;
uniform float maxPres;

uniform float maxSurfaceTension;

uniform vec3 solidColor;

out vec4 FragColor;

vec3 velocityToHSV(vec2 v, float maxSpeed)
{
    float angle = atan(v.y, v.x);
    float hue = (angle + PI) / (2.0 * PI);

    float value = 1.0 - exp(-2.1 * length(v));

    float saturation = 1.0;

    return vec3(hue, saturation, value);
}

vec3 hsv2rgb(vec3 c)
{
    vec3 rgb = clamp(
    abs(mod(c.x * 6.0 + vec3(0.0,4.0,2.0), 6.0) - 3.0) - 1.0,
    0.0,
    1.0
    );

    rgb = rgb * rgb * (3.0 - 2.0 * rgb);
    return c.z * mix(vec3(1.0), rgb, c.y);
}

vec3 indexToColor(float i) {
    float r = fract(sin(float(i) * 12.9898) * 43758.5453);
    float g = fract(sin(float(i) * 78.233)  * 43758.5453);
    float b = fract(sin(float(i) * 39.425)  * 43758.5453);
    return vec3(r, g, b);
}

void main() {
    if (selectedParticleID == Properties.w) {
        FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
        return;
    }

//    if (Flags.x == 0.0) {
//        discard;
//        FragColor = vec4(0.0, 1.0f, 0.0f, 1.0);
//        return;
//    }

//    if (Flags.x == 1.0f) {
//        FragColor = vec4(1.0);
//        return;
//    } else {
//        FragColor = vec4(1.0, 0.0, 1.0, 1.0);
//        return;
//    }

//    if (Debugging.x > 0.0f) {
//        FragColor = vec4(1.0);
//        return;
//    }

    // SOLID COLOR PARTICLES
    if (mode == 0) {
        FragColor = Color;
        return;
    }

    // VELOCITY PARTICLES
    if (mode == 1) {
        float speed = length(Velocity);
        float t = (speed - minVel) / (maxVel - minVel + 1e-6f);
        FragColor = mix(minVelColor, maxVelColor, t);
        return;
    }

    // PRESSURE PARTICLES
    if (mode == 2) {
        float t = (Properties.z - minPres) / (maxPres - minPres);
        FragColor = mix(minPresColor, maxPresColor, t);
        return;
    }

    // SURFACE PARTICLES
    if (mode == 3) {
        float t = clamp(Flags.x / maxSurfaceTension, 0.0, 1.0);
        FragColor = mix(Color, vec4(1.0), t);
//        if (Flags.x > 0.5) {
//            FragColor = vec4(1.0);
//        } else {
//            FragColor = particleColor;
//        }
        return;
    }
}