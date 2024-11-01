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
};

layout(std140) uniform lights{
   Light LightList[MAX_LIGHT_CNT];
};

uniform int LightCount;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform vec3 viewPos;

void main(){

    vec3 fragPos = texture(gPosition,TexCoords).rgb;
    vec3 normal = texture(gNorrmal,TexCoords).rgb;
    vec3 normal = texture(gAlbedoSpec,TexCoords).rgb;

    vec3 viewDir = normalize(viewPos - fragPos);

    for(int i = 0; i < MAX_LIGHT_CNT; ++i){

        float distance = length(LightList[i].position - fragPos);
        if(distance )

    }

}