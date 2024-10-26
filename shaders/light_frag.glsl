#version 410
// Output for on-screen color
out vec4 outColor;

/*
struct Light {
    vec3 position;
    vec3 color;
    vec3 direction;

    bool is_spotlight;
    bool has_texture;
};

const int MAX_LIGHTS = 10;

layout(std140) uniform LightBlock {
    Light lights[MAX_LIGHTS];
};
*/

uniform vec3 color;
//uniform int numLights;

void main() {
    outColor = vec4(color, 1.0);
}