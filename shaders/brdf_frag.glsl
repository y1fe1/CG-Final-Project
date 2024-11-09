#version 410 core

out vec2 FragColor;

in vec2 TexCoords;

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits){
	bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint n){
	return vec2(float(i)/float(n), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 normal, float roughness)
{
	float a = roughness*roughness;
	
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
	
    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);
	
    vec3 sampleVec = tangent * H.x + bitangent * H.y + normal * H.z;
    return normalize(sampleVec);
}

// Schlick-GGX approximation
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 2.0;

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

vec2 IntegrateBRDF(float NdotV, float roughness) {
    
    vec3 viewDir = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);

    float diffuseTerm = 0.0;
    float specularTerm = 0.0;

    vec3 surfaceNormal = vec3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        
        vec2 Xi = Hammersley(i, SAMPLE_COUNT); // Low discrepancy sequence
        vec3 halfwayDir = ImportanceSampleGGX(Xi, surfaceNormal, roughness);
        vec3 lightDir = normalize(2.0 * dot(viewDir, halfwayDir) * halfwayDir - viewDir);

        float NdotL = max(lightDir.z, 0.0);
        float NdotH = max(halfwayDir.z, 0.0);
        float VdotH = max(dot(viewDir, halfwayDir), 0.0);

        if (NdotL > 0.0) {
            float G = GeometrySmith(surfaceNormal, viewDir, lightDir, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float fresnelCoefficient = pow(1.0 - VdotH, 5.0);

            diffuseTerm += (1.0 - fresnelCoefficient) * G_Vis;
            specularTerm += fresnelCoefficient * G_Vis;
        }
    }

    diffuseTerm /= float(SAMPLE_COUNT);
    specularTerm /= float(SAMPLE_COUNT);
    
    return vec2(diffuseTerm, specularTerm);
}


void main() 
{
    vec2 integratedBrdf = IntegrateBRDF(TexCoords.x, TexCoords.y);
    FragColor = integratedBrdf;
}