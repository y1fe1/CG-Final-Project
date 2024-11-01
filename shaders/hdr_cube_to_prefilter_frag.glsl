#version 410 core

out vec4 fragColor;

in vec3 localPos;

uniform samplerCube environmentMap;
uniform float roughness;

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

float DistributionGGX(float NdotH, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH2 = NdotH * NdotH;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

void main()
{
	vec3 normal = normalize(localPos);
	vec3 R = normal;
	vec3 V = R;

	const uint SAMPLE_COUNT = 1024u;
	float totalWeight = 0.0;

	vec3 prefilteredColor = vec3(0.0);

	for(uint i = 0u; i < SAMPLE_COUNT; ++i){
		
		vec2 Xi = Hammersley(i, SAMPLE_COUNT); // low discrepeacny value
		vec3 H = ImportanceSampleGGX(Xi, normal,roughness);
        vec3 L = normalize(2.0*dot(V,H) * H - V);
    

        float NdotL = max(dot(normal,L), 0.0);
        if(NdotL > 0.0){
            
            if(false){

                // Calculate PDF and mip level for roughness
                float resolution = 512.0;
                float NdotH = max(dot(normal, H), 0.0);
                float HdotV = max(dot(H, V), 0.0);
                float D     = DistributionGGX(NdotH, roughness);
                float pdf   = (D * NdotH / (4.0 * HdotV)) + 0.0001;

                float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
                float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
                float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

                // Sample the environment map at the calculated mip level
                prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
                totalWeight += NdotL;
            }

            prefilteredColor += texture(environmentMap, L).rgb * NdotL;
            totalWeight      += NdotL;
        }
	}

    prefilteredColor = prefilteredColor/totalWeight;

    fragColor = vec4(prefilteredColor,1.0);
}