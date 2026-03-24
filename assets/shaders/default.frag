#version 430 core

#define PI 3.1415

in vec2 Velocity;
in vec2 TexCoord;
in vec4 Properties;
flat in int ID;

out vec4 FragColor;

uniform int selectedParticleID = -1;

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
    vec2 coord = TexCoord - vec2(0.5);
    float dist = length(coord);

    if (dist > 0.5)
        discard;

    if (selectedParticleID == ID) {
        FragColor = vec4(1.0f);
        return;
    }

    FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
//        FragColor = vec4(Properties.w / 50f, 0.0f, 0.0f, 1.0f);
    //    FragColor = vec4(hsv2rgb(velocityToHSV(Velocity, 10.0)), 1.0f);
    //    FragColor = vec4(indexToColor(Properties.w), 1.0f);
}