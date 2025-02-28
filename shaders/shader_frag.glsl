#version 410

#define MAX_LIGHT_CNT 10

// Material Setting
layout(std140) uniform Material // Must match the GPUMaterial defined in src/mesh.h
{
    vec3 kd;
	vec3 ks;
	float shininess;
	float transparency;
};

// Shadow Setting
layout(std140) uniform shadowSetting{
    bool shadowEnabled;
    bool pcfEnabled;
    bool _UNUSE_PADDING6;
    bool _UNUSE_PADDING7;
};
uniform sampler2D texShadow;

//Light Setting
layout(std140) uniform Light {
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

uniform mat4 lightMVP;

uniform vec3 viewPos;

uniform sampler2D colorMap;
uniform bool hasTexCoords;
uniform bool useMaterial;

uniform bool ignoreLightDirection;
uniform float sunlightStrength;
uniform bool useNormalMapping;
uniform bool useParallaxMapping;
uniform sampler2D normalTex;
uniform sampler2D heightTex;

uniform vec3 ambientColor;

uniform mat3 normalModelMatrix;

//Env Mapping
uniform samplerCube SkyBox;
uniform bool useEnvMap;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in mat3 TBN;

layout(location = 0) out vec4 fragColor;


float shadowFactorCal(vec2 shadowMapCoord, float fragLightDepth){

    const float bias = 0.005; 

    // Apply shadow factor
    float shadowFactor = 0;

    if(!pcfEnabled)
    {
         // Retrieve the shadow map depth value at this coordinate
        float shadowMapDepth = texture(texShadow, shadowMapCoord).x;
        shadowFactor = (fragLightDepth > shadowMapDepth + bias) ? 1.0: 0.0; // Shadow factor
    } 

    else if(pcfEnabled)
    {
        // PCF 
        float shadowSum = 0.0;
        float sampleCount = 9.0f; // Total number of samples
        vec2 texelSize  = 1.0/textureSize(texShadow,0); // Radius for PCF sampling

        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                vec2 offset = vec2(float(x), float(y)) * texelSize;
                float shadowMapDepth = texture(texShadow, shadowMapCoord + offset).r;
                shadowSum += (fragLightDepth > shadowMapDepth + bias) ? 1.0 : 0.0;
            }
         }

        // Average the shadow factors
        shadowFactor = shadowSum / sampleCount;
    }


    return shadowFactor;
}

float getLightAttenuationFactor(vec3 lightDir) {
    float dist = length(lightDir);
    float attenuation = 1.0 / (1.0 + (linear * dist) + (quadratic * dist * dist)); // Simple quadratic falloff

    // Clamp the attenuation to avoid excessively bright values at close distances
    return clamp(attenuation, 0.0, 1.0);
}

float getHeightFromMap(vec2 texPos) {
    vec3 height = texture(heightTex, texPos).rgb;
    return height.r;
}

vec2 parallaxTexCoords() {
    mat3 trTBN = transpose(TBN);
    vec3 viewDir = normalize(trTBN * viewPos - trTBN * fragPosition);

    float layers = mix(256.0, 8.0, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));

    vec2 delta = (viewDir.xy / viewDir.z) * 0.025 / layers;

    vec2 currentCoords = fragTexCoord;
    float currentHeightMapVal = getHeightFromMap(fragTexCoord);

    float layerDepth = 1.0 / layers;
    float currentHeight = 0.0;

    while (currentHeight < currentHeightMapVal) {
        currentCoords -= delta;
        currentHeightMapVal = getHeightFromMap(currentCoords);
        currentHeight += layerDepth;
    }

    vec2 prevCoords = currentCoords + delta;
    float beforeDepth = getHeightFromMap(prevCoords) - currentHeight + layerDepth;
    float afterDepth = currentHeightMapVal - currentHeight;

    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 parallaxCoords = prevCoords * weight + currentCoords * (1.0 - weight);
    parallaxCoords = vec2(clamp(parallaxCoords.x, 0.0, 1.0), clamp(parallaxCoords.y, 0.0, 1.0));
    return parallaxCoords;
}

void main()
{
    vec2 texCoord = useParallaxMapping ? parallaxTexCoords() : fragTexCoord;
    vec3 normal;
    
    if (useNormalMapping)
    {
        normal = texture(normalTex, texCoord).rgb; // Take the normal from the normal map texture.
        normal = normal * 2.0 - 1.0; // Re-convert from [0, 1] to [-1, 1].
        normal = normalize(TBN * normal); // Transform with TBN.
    }
    else
    {
        // Take the given normal from the vertex shader instead.
        normal = normalize(fragNormal);
    }

    vec4 fragLightCoord = lightMVP * vec4(fragPosition, 1.0);
    // Convert to normalized device coordinates
    fragLightCoord.xyz /= fragLightCoord.w; // Homogeneous divide
    
    // Transform from NDC to texture space (0 to 1)

    fragLightCoord.xyz = fragLightCoord.xyz * 0.5 + 0.5;
    
    // Fragment depth in light space
    float fragLightDepth = fragLightCoord.z;

    // Shadow map coordinates (XY)
    vec2 shadowMapCoord = fragLightCoord.xy;
    float shadowFactor = (shadowEnabled) ? shadowFactorCal(shadowMapCoord, fragLightDepth) : 0.0f;

    vec3 Specular = vec3(0.0f);

    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 lightDir = normalize(position - fragPosition);

    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfDir = normalize(lightDir + viewDir);

    float lambert = ignoreLightDirection ? abs(dot(normal, lightDir)) : max(dot(normal, lightDir), 0.0);
    //vec3 ambient = ambientColor;
    
    vec3 finalColor = vec3(0.0f);

    //texColor
    vec4 texColor = vec4(0.0f);

    if (hasTexCoords) {
       texColor = texture(colorMap, texCoord); 
    }

    // Env Mapping
    vec3 envReflectDir = vec3(0.0f);
    vec3 envColor = vec3(0.0f);
    vec3 finalEnvColor = vec3(0.0f);

    if (useEnvMap)  {
        // Environment mapping with reflection
        envReflectDir = reflect(-viewDir, normal); // Reflection vector for env mapping
        envColor = texture(SkyBox, envReflectDir).rgb;

        // Combine environment color with material color
        finalEnvColor = (hasTexCoords) ? envColor * texColor.rgb : envColor;
    }

    if (useMaterial) { 
        
        //lambert factor
        vec3 diffuse = (hasTexCoords) ? lambert * kd * texColor.rgb : lambert * kd;
        
        //basic blinn-phong model
        if(lambert >= 0.0 && !ignoreLightDirection && shininess > 0.001) {
            Specular = ks * pow(max(dot(halfDir, normal), 0.0f), shininess);
            Specular = (hasTexCoords)? texColor.rgb * Specular : Specular;
        }
        
        // Calculate the light attenuation factor based on distance
        float lightAttenuationFactor = getLightAttenuationFactor(lightDir);

        finalColor = Specular + color * diffuse;
        finalColor = finalColor * (1.0 - shadowFactor) * lightAttenuationFactor * sunlightStrength;
    }

    float reflectionStrength = 1.0f;

    if (useMaterial && useEnvMap) {
        // Blend material shading with environment map for a reflective effect
        fragColor = vec4(mix(finalColor, finalEnvColor, reflectionStrength), 1.0);
    }
    else if (useMaterial) {
        // Only material shading
        fragColor = vec4(finalColor, 1.0);
    }
    else if (useEnvMap) {
        // Only environment mapping
        fragColor = vec4(envColor, 1.0);
    }
    else if (hasTexCoords) {
        // Fallback: use texture color only if available
        fragColor = texColor;
    }
    else { 
        // Display the normal if nothing else is selected
        fragColor = vec4(normal, 1.0); 
    }
}


