#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

smooth out vec3 smoothNormal;
flat out vec3 flatNormal;
out vec3 fragPos;  

uniform mat4 MVP;

void main(){
    gl_Position = MVP * vec4(aPos, 1.0);
    smoothNormal = aNormal;
    flatNormal = aNormal;
    fragPos = aPos;
}