#version 430 core

#define PI 3.1415

in vec3 Velocity;
in vec4 Properties;
//in vec2 TexCoord;
//in vec4 Flags;
//in vec4 Debugging;
flat in int ID;

out vec4 FragColor;

uniform int selectedParticleID = -1;

// --- Rendering modes ---
uniform int mode = 0;

uniform vec3 particleColor;

uniform vec3 minVelColor;
uniform float minVel;
uniform vec3 maxVelColor;
uniform float maxVel;

uniform vec3 minPresColor;
uniform float minPres;
uniform vec3 maxPresColor;
uniform float maxPres;

uniform vec3 solidColor;

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
    if (selectedParticleID == ID) {
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
        FragColor = vec4(particleColor, 1.0);
        return;
    }

    // VELOCITY PARTICLES
    if (mode == 1) {
        float speed = length(Velocity);
        float t = (speed - minVel) / (maxVel - minVel + 1e-6f);
        vec3 color = mix(minVelColor, maxVelColor, t);
        FragColor = vec4(color, 1.0);
    }

    // PRESSURE PARTICLES
    if (mode == 2) {
        vec3 color;
        if (Properties.z < 0) {
            float t = clamp(abs(Properties.z) / abs(minPres), 0.0f, 1.0f);
            color = mix(vec3(0.5, 0.5, 1.0), minPresColor, t); // light blue to dark blue
        } else {
            float t = clamp(Properties.z / maxPres, 0.0f, 1.0f);
            color = mix(vec3(1.0, 0.5, 0.5), maxPresColor, t); // light red to dark red
        }
        FragColor = vec4(color, 1.0);
    }

    // SOLID COLOR
    if (mode == 3) {
        FragColor = vec4(solidColor, 1.0);
        return;
    }

//        FragColor = vec4(1.0, 0.0, 0.0, 1.0f);
    //    FragColor = vec4(1.0f, 0.0f, Debugging.x, 1.0f);
//        FragColor = vec4(indexToColor(Debugging.y), 1.0f);
    //    FragColor = vec4(hsv2rgb(velocityToHSV(Velocity, 10.0)), 1.0f);
    //    FragColor = vec4(indexToColor(Properties.w), 1.0f);
}