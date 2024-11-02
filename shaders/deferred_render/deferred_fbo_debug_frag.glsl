#version 410 core

out vec4 finalColor;
in  vec2 TexCoords;

uniform sampler2D fboDebug;

void main()
{
	finalColor = texture(fboDebug, TexCoords);
}