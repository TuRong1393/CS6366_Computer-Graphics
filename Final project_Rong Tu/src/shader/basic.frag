#version 330 core

layout(location = 0) out vec4 fragColor;	//fragment shader output

smooth in vec3 outNormal;	

//uniform
uniform vec4 colObject;  //object color
uniform bool bNormalAsColor; //normal as color sign
uniform float fAlpha; //alpha (normal as color sign is true)

void main(){
	if(bNormalAsColor) {
		//output the object space normal as color
		fragColor = vec4(outNormal, fAlpha);
	} else {
		fragColor = colObject;
	}
}
