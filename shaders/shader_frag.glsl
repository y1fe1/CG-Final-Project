#version 410

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
    bool transparencyEnabled;
};
uniform sampler2D texShadow;

//Light Setting
layout(std140) uniform Light {
    vec3 position;
    vec3 color;
    vec3 direction;

    bool is_spotlight;
    bool has_texture;
};
uniform mat4 lightMVP;

uniform vec3 viewPos;

uniform sampler2D colorMap;
uniform bool hasTexCoords;
uniform bool useMaterial;

uniform vec3 ambientColor;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;


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

void main()
{

    vec4 fragLightCoord = lightMVP * vec4(fragPosition, 1.0);
    // Convert to normalized device coordinates
    fragLightCoord.xyz /= fragLightCoord.w; // Homogeneous divide
    
    // Transform from NDC to texture space (0 to 1)

    fragLightCoord.xyz = fragLightCoord.xyz * 0.5 + 0.5;
    
    // Fragment depth in light space
    float fragLightDepth = fragLightCoord.z;

    // Shadow map coordinates (XY)
    vec2 shadowMapCoord = fragLightCoord.xy;

    vec3 Specular = vec3(0.0f);
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(position - fragPosition);
    vec3 viewDir = normalize(viewPos - fragPosition);

    float lambert = max(dot(normal,lightDir),0.0);
    vec3 ambient = ambientColor;

    float shadowFactor = (shadowEnabled)? shadowFactorCal(shadowMapCoord,fragLightDepth) : 0.0f;

    if (hasTexCoords)       { 
        fragColor = vec4(texture(colorMap, fragTexCoord).rgb, 1);
    }
    else if (useMaterial)   { 
        
        //lambert factor
        vec3 diffuse = lambert*kd;
        
        //basic phong model
        if(lambert > 0.0f) {
            vec3 reflectDir = reflect(-lightDir, normal);
            Specular = ks * pow(max(dot(reflectDir, viewDir), 0.0f), shininess);
        }
        
        vec3 finalColor = (ambient + diffuse + Specular);

        fragColor = vec4(finalColor * color * (1-shadowFactor), 1);
    }
    else                    { fragColor = vec4(normal, 1); } // Output color value, change from (1, 0, 0) to something else
}


