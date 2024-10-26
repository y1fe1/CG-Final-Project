#version 410

uniform vec4 pos; // Screen-space position

/*
uniform int numLights;
uniform mat4 mvp;

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

void main()
{
    gl_Position = pos;
}

