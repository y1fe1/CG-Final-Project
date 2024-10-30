#version 410 core
out vec4 FragColor;

in vec3 fragTexCoords;

uniform samplerCube skybox;

void main()
{    
    FragColor = texture(skybox, fragTexCoords);
    //FragColor = vec4(fragTexCoords,1.0);
}