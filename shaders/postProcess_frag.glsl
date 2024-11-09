#version 410
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;

const float threshold = 0.6;   // Threshold for high light area
const float blur_offset = 5 / 1024.0; // Bias for blur

void main()
{
    // Step 1: Extract Bright Parts
    vec3 color = texture(scene, TexCoords).rgb;
    vec3 brightParts = max(color - vec3(threshold), 0.0); // High light area

    // Step 2: Apply Gaussian Blur to the Bright Parts
    vec3 bloom = vec3(0.0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            bloom += texture(scene, TexCoords + vec2(x, y) * blur_offset).rgb * brightParts;
        }
    }
    bloom /= 9.0; // Average

    // Step 3: Combine Bloom with Original Scene
    vec3 finalColor = color + bloom;
    FragColor = vec4(finalColor, 1.0);
}
