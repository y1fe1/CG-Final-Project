#version 410 core

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube hdrEnvMap;

void main()
{
    vec3 envColor = texture(hdrEnvMap, TexCoords).rgb;
    
    // gamma correction
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2)); 
  
    FragColor = vec4(envColor, 1.0);
    //FragColor = vec4(TexCoords,1.0);
}