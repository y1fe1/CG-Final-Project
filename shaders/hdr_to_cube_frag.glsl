#version 410

in vec3 localPos;

out vec4 FragColor;

uniform sampler2D equirectangularMap;
const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{	

    const float gamma = 2.2;

    vec2 uv = SampleSphericalMap(normalize(localPos)); // make sure to normalize localPos
    vec3 color = texture(equirectangularMap, uv).rgb;

    // reinhard tone mapping 
    color = color / (color + vec3(1.0));

    // gamma correction
    //color = pow(color, vec3(1.0/gamma));

    // this two step help generate better irradiance mapping without losing details
    
    FragColor = vec4(color, 1.0);
}