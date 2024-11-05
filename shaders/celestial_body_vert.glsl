#version 410

uniform mat4 mvpMatrix;
uniform vec3 viewPos;   // cameraPos -  calc.:  fCameraHeight, fCameraHeight2
uniform vec3 lightPos;  //              calc.:  v3LightDir
uniform float outerRadius;  //          calc.:  fOuterRadius2       (radius of atmosphere)
uniform float innerRadius;  //          calc.:  fInnerRadius2       (radius of planet)

const vec3 wavelength           = vec3(1.0, 1.0, 1.0);  // ???
const vec3 inverseWavelength    = 1.0 / pow(wavelength, 4.0);
const float E_SUN               = 1.0;  // ???
const float RAYLEIGH            = 1.0;  // Kr
const float MIE                 = 1.0;  // Km
const float SCALE_DEPTH         = 1.0;  // fScaleDepth???

const float PI                  = 3.14159265359;
const uint SAMPLES              = 100;

const float RAYLEIGH_PI_4       = RAYLEIGH * PI * 4.0;
const float MIE_PI_4            = MIE * PI * 4.0;

layout(location = 0) in vec3 position;

out vec3 lightDir;
out vec3 FragPos;
out vec3 c0;
out vec3 c1;
out vec3 t0;

float nearestIntersection(vec3 pos, vec3 ray, float distSq, float rSq)
{
    float b = 2.0 * dot(pos, ray);
    float bSq = b * b;
    float c = distSq - rSq;
    float dRt = sqrt(max(0.0, bSq - 4.0 * c));
    return 0.5 * (-b - dRt);
}

float expScale(float cosAng)
{
    float x = 1.0 - cosAng;
    return SCALE_DEPTH * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.8 + x * 5.25))));
}

void calculateAtmosphere()
{
    vec3 ray = position - viewPos;
    float rayFar = length(ray);
    ray /= rayFar;

    camHeightSq = viewPos.y * viewPos.y;
    outerRadiusSq = outerRadius * outerRadius;
    
    float rayNear = nearestIntersection(viewPos, ray, camHeightSq, outerRadiusSq);
    rayFar -= rayNear;

    float scale = 1.0 / (outerRadius - innerRadius);

    vec3 start = viewPos + ray * rayNear;
    float startAngle = dot(ray, start) / outerRadius;
    float startDepth = exp(-1.0 / SCALE_DEPTH);
    float startOffset = startDepth * expScale(startAngle);

    float sampleLength = rayFar / ((float)SAMPLES);
    float scaledLength = sampleLength * scale;
    vec3 sampleRay = ray * sampleLength;
    vec3 samplePoint = start + sampleRay * 0.5;

    float scaleOverDepth = scale / SCALE_DEPTH;
    lightDir = normalize(lightPos - position);

    vec3 frontColor = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < SAMPLES; ++i)
    {
        float height = length(samplePoint);
        float depth = exp(scaleOverDepth * (innerRadius - height));

        float lightAngle = dot(lightDir, samplePoint) / height;
        float cameraAngle = dot(ray, samplePoint) / height;
        float scatter = startOffset + depth * (expScape(lightAngle) - expScape(cameraAngle));

        float attenuate = exp(-scatter * (inverseWavelength * RAYLEIGH_PI_4 + MIE_PI_4));
        frontColor += attenuate * (depth + scaledLength);
        samplePoint += sampleRay;
    }

    FragPos = mvpMatrix * position;
    c0 = frontColor * inverseWavelength * RAYLEIGH * E_SUN;
    c1 = frontColor * MIE * E_SUN;
    t0 = viewPos - position;
}

void main()
{
    calculateAtmosphere();
    gl_Position = vec4(FragPos, 1.0);
}
