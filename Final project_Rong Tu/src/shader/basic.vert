#version 330 core
  
layout(location = 0) in vec3 aPos; //object space vertex position
layout(location = 1) in vec3 vNormal;	//object space vertex normal

uniform mat4 MVP;   //MVP matrix

smooth out vec3 outNormal;

void main()
{  
	gl_Position = MVP * vec4(vec3(1.0) - aPos, 1);
	outNormal = vNormal;
}