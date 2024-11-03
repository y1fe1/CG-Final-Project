#version 410

layout(location = 0) out vec3 gPos;
layout(location = 1) out vec3 gNor;
layout(location = 2) out vec4 gCol;

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoords;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_specular;

void main()
{
	gPos = fragPos;
	gNor = normalize(fragNormal);
	gCol.rgb = texture(texture_diffuse, fragTexCoords).rgb;
	gCol.a = texture(texture_specular, fragTexCoords).r;
}
