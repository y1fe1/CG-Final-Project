#version 410
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;

void main()
{
    // Simple Vague
    vec3 color = vec3(0.0);
    float offset = 1.0 / 300.0;

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            color += texture(scene, TexCoords + vec2(x, y) * offset).rgb;
        }
    }
    color /= 9.0; // Get Average
    FragColor = vec4(color, 1.0);
}
