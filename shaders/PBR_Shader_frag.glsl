#version 410

precision highp float;

#define MAX_LIGHT_CNT 10
#define PI 3.1415926535897932384626433832795

// Implementation of PBR Shading

// PBR Material 
layout(std140) uniform PBR_Material
{
    vec3 albedo;
    float metallic;
    float roughness;
    float ao; // Ambiend Occlusion

    vec3 _UNUSE_PADDING0;
};

// Shadow Setting
layout(std140) uniform shadowSetting{
    bool shadowEnabled;
    bool pcfEnabled;
};

uniform sampler2D texShadow;

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

uniform int LightCount;

uniform mat4 lightMVP;

uniform vec3 viewPos;

uniform sampler2D normalMap;
uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

uniform bool hasTexCoords;
uniform bool useMaterial;

uniform vec3 ambientColor;

uniform bool hdrEnvMapEnabled;
uniform samplerCube irradianceMap;
uniform samplerCube prefilteredMap;
uniform sampler2D brdfLUT;

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

float getLightAttenuationFactor(vec3 lightDir) {
    float dist = length(lightDir);
    float attenuation = 1.0 / (dist * dist); // Simple quadratic falloff

    // Clamp the attenuation to avoid excessively bright values at close distances
    return clamp(attenuation, 0.0, 1.0);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

// introduce roughness for Fresnel-Schlick equation
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   

//GGX Distribution
float DistributionGGX(vec3 normal, vec3 halfDir, float roughness)
{
    float a      = roughness*roughness;
    float NdotH  = max(dot(normal, halfDir), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a*a;
    float denom = (NdotH2 * (a*a - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

// Schlick-GGX approximation
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

// Schlick-GGX approximation
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float g_n_v_k  = GeometrySchlickGGX(NdotV, roughness);
    float g_n_l_k  = GeometrySchlickGGX(NdotL, roughness);
	
    return g_n_v_k * g_n_l_k;
}

vec3 normalFromNormalMapGetter(sampler2D normalMap){

    vec3 tangentNormal = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;

    vec3 Q1 = dFdx(fragPosition);
    vec3 Q2  = dFdy(fragPosition);
    vec2 st1 = dFdx(fragTexCoord);
    vec2 st2 = dFdy(fragTexCoord);

    vec3 N   = normalize(fragNormal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}


void main()
{

    vec3 Albedo = albedo;
    vec3 normal = normalize(fragNormal);
    float Metallic = metallic;
    float Roughness = roughness;
    float Ao = ao;

    vec3 ambient = vec3(0.03);
    vec3 irradiance = vec3(0.0);
    vec3 diffuse = vec3(0.0);
    if (hasTexCoords)       { 
        //fragColor = vec4(texture(colorMap, fragTexCoord).rgb, 1);

        Albedo = pow(texture(albedoMap,fragTexCoord).rgb, vec3(2.2));
        normal = normalFromNormalMapGetter(normalMap);
        Metallic  = texture(metallicMap, fragTexCoord).r;
        Roughness = texture(roughnessMap, fragTexCoord).r;
        Ao        = texture(aoMap, fragTexCoord).r;

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

    float shadowFactor = (shadowEnabled)? shadowFactorCal(shadowMapCoord,fragLightDepth) : 0.0f;

    vec3 LightIntensity = vec3(0.0f);

    vec3 Specular = vec3(0.0f);
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 reflectDir = (-viewDir,normal);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, Albedo, Metallic);

    vec3 finalColor = vec3(0.0f);

    if (useMaterial) { 
        
        for (int idx = 0; idx < LightCount; ++idx){

            vec3 lightDir = normalize(LightList[idx].position - fragPosition);
            vec3 halfDir = normalize(viewDir + lightDir);

            float distance = length(LightList[idx].position - fragPosition);
            // Calculate the light attenuation factor based on distance
            float lightAttenuationFactor = getLightAttenuationFactor(lightDir);

            vec3 radiance = LightList[idx].color * lightAttenuationFactor;

            // Cook Tolerance
            float NDF = DistributionGGX(normal, halfDir, Roughness);
            float G = GeometrySmith(normal,viewDir,lightDir,Roughness);
            vec3 F = fresnelSchlick(max(dot(halfDir, viewDir), 0.0), F0);

            vec3 kS = F;
            vec3 kD = vec3(0.0);

            kD = vec3(1.0)-kS;
            kD *= 1.0 - Metallic;

            vec3 numerator    = NDF * G * F;
            float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
            vec3 specular     = numerator / denominator; 

            float NdotL = max(dot(normal, lightDir), 0.0);                

            finalColor += (kD * Albedo / PI + specular) * radiance * NdotL;
            //finalColor += radiance;
        }

        vec3 color = vec3(0.0);

        if(!hdrEnvMapEnabled){
            ambient = ambient * Albedo * Ao;
        } 

        else {
            vec3 kS = fresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, Roughness); 
            vec3 kD = vec3(1.0)-kS;
            irradiance = texture(irradianceMap, normal).rgb;
            diffuse = irradiance * Albedo;

            const float MAX_REFLECTION_LOD = 4.0;
            vec3 perfilteredColor = textureLod(prefilteredMap,reflectDir, Roughness * MAX_REFLECTION_LOD).rgb; 
            vec2 brdf = texture(brdfLUT, vec2(max(dot(normal, viewDir), 0.0), Roughness)).rg;
            vec3 specular = perfilteredColor * (kS * brdf.x + brdf.y);

            ambient = (kD * diffuse + specular) * Ao;
        }

        color = ambient + finalColor;
        // gamma correction
        float gamma = 2.2f;

        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0/gamma));  
   
        fragColor = vec4(color, 1.0);
    }
    else {
        fragColor = vec4(normal, 1); 
    } // Output color value, change from (1, 0, 0) to something else

}


