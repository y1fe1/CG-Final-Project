#version 410 
uniform mat4 mvpMatrix;

out vec3 gPosition;
out vec3 gNormal;
out vec3 gAlbedoSpec;

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoords;

void main()
{
	gPosition = fragPos;

	gNormal = normalize(fragNormal);

	gAlbedoSpec.rgb = vec3(0.95);
	
	gl_Position = mvpMatrix * vec4(fragPos, 1);
}