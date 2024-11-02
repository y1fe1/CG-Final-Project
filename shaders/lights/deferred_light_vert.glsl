#version 410 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoords;

uniform mat4 mvpMatrix;
uniform vec4 pos;

void main(){
	gl_Position = mvpMatrix * vec4(fragPos,1.0);
}