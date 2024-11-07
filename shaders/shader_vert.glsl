#version 410

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
// Normals should be transformed differently than positions:
// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
uniform mat3 normalModelMatrix;
uniform bool hasTexCoords;
uniform bool useNormalMapping;
uniform bool useParallaxMapping;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoord;
out mat3 TBN; // Used in normal mapping

mat3 generateTBN()
{
    // Get two edges by adding the normal to the vertex position.
    vec3 edge1      = vec3(modelMatrix * vec4(position + normal, 1.0)) - position;
    vec3 edge2      = vec3(modelMatrix * vec4(position + vec3(normal.y, -normal.x, normal.z), 1.0)) - position;
    // Assume two arbitrary UV edges.
    vec2 deltaUV1   = vec2(1.0, 0.0);
    vec2 deltaUV2   = vec2(0.0, 1.0);
    // Calculate tangent and bitangent.
    vec3 tangent    = normalize(edge1 * deltaUV2.t - edge2 * deltaUV1.t);
    vec3 bitangent  = normalize(edge2 * deltaUV1.s - edge1 * deltaUV2.s);

    // Approximate the texture coordinate at the given vertex position.
    // This is only necessary if the texture coordinates are not given.
    if (!hasTexCoords)
    {
        fragTexCoord = vec2(dot(position, tangent), dot(position, bitangent));
    }

    return mat3(tangent, bitangent, normal);
}

void main()
{
    gl_Position = mvpMatrix * vec4(position, 1);
    
    fragPosition    = (modelMatrix * vec4(position, 1)).xyz;
    fragNormal      = normalModelMatrix * normal;
    fragTexCoord    = texCoord;

    if (useNormalMapping || useParallaxMapping)
    {
        TBN = generateTBN();
    }
}
