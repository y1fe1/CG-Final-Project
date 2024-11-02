#version 410 

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragTexCoords;

void main()
{
	vec4 worldPos =  model * vec4(position,1.0);
	fragPos = worldPos.xyz;

	fragNormal = normalMatrix * normal;

	fragTexCoords = texCoord;
	
	gl_Position = projection * view * worldPos;
}