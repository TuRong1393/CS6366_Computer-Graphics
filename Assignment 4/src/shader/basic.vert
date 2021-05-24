#version 330 core
  
layout(location = 0) in vec3 aPos; //object space vertex position

uniform mat4 MVP;   //MVP matrix

smooth out vec3 vUV; //3D texture coordinates for texture lookup in the fragment shader

void main()
{  
	gl_Position = MVP * vec4(aPos,1);
	vUV = aPos;
}