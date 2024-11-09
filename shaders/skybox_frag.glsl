#version 410 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    vec4 texColor = texture(skybox, TexCoords);
    FragColor = texColor;
}