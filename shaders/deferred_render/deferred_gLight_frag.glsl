#version 410 core

#define MAX_LIGHT_CNT 10

//Light Setting
struct Light {
    vec3 position;
    float _UNUSE_PADDING0;

    vec3 color;
    float _UNUSE_PADDING1;

    vec3 direction;
    float _UNUSE_PADDING2;

    bool is_spotlight;
    bool has_texture;
    vec2 _UNUSE_PADDING3;

    float linear;
    float _UNUSE_PADDING4;  
    float quadratic;
    float radius;
};

layout(std140) uniform lights{
   Light LightList[MAX_LIGHT_CNT];
};

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform vec3 viewPos;

in vec2 TexCoords;

out vec4 outColor;

float getLightAttenuationFactor(Light curLight,vec3 lightDir) {
    float dist = length(lightDir);
    float attenuation = 1.0 / (1.0+ curLight.linear * dist + curLight.quadratic*dist * dist); // Simple quadratic falloff

    // Clamp the attenuation to avoid excessively bright values at close distances
    return clamp(attenuation, 0.0, 1.0);
}


void main(){

    vec3 fragPos = texture(gPosition,TexCoords).rgb;
    vec3 normal = texture(gNormal,TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec,TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 finalColor = Diffuse * 0.1;  // simple ambient

    for(int i = 0; i < MAX_LIGHT_CNT; ++i){

        float distance = length(LightList[i].position - fragPos);
        if(distance < LightList[i].radius){
            // diffuse
            vec3 lightDir = normalize(LightList[i].position - fragPos);
            vec3 diffuse = max(dot(normal, lightDir), 0.0) * Diffuse * LightList[i].color;

            // specular
            vec3 halfwayDir = normalize(lightDir + viewDir);  
            float spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
            vec3 specular = LightList[i].color * spec * Specular;
            // attenuation
            float attenuation = getLightAttenuationFactor(LightList[i],lightDir);

            diffuse *= attenuation;
            specular *= attenuation;
            finalColor += diffuse + specular;
        }

    }

    outColor = vec4(finalColor,1.0);
}