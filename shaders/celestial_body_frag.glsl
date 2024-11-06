#version 410

const float G1 = 0.75;   // ???
const float G2 = 0.8;   // ???

in vec3 lightDir;
in vec3 FragPos;
in vec3 c0;
in vec3 c1;
in vec3 t0;

out vec4 FragColor;

void main()
{
    float cosAng = dot(lightDir, t0) / min(0.01, length(t0));
    float cosSq = cosAng * cosAng;
    
    float miePhase = 1.5 * ((1.0 - G2) / (2.0 + G2)) * (1.0 + cosSq) / min(0.01, pow(1.0 + G2 - 2.0 * G1 * cosAng, 1.5));
    float rayleighPhase = 0.75 * (1.0 + G2);

    vec4 outputColor = vec4(1.0, 1.0, 1.0, 1.0);
    outputColor.xyz = (rayleighPhase * c0) + (miePhase * c1);
    outputColor.w = outputColor.z;

    FragColor = outputColor;
}
