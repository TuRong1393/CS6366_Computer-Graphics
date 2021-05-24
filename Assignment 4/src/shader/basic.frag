#version 330 core

layout(location = 0) out vec4 fragColor;	//fragment shader output

smooth in vec3 vUV;	

//uniform
uniform vec3 colObject;
uniform sampler3D volTexture;		//volume dataset
uniform sampler1D tfTexture;		//transfer function
uniform bool bTransferFunctionSign; //transfer function sign

void main(){
    if(bTransferFunctionSign) {
        fragColor = texture(tfTexture, texture(volTexture, vec3(1.0) - vUV).x);
    } else {
        fragColor = vec4(colObject, 1.0f);
    }
}
