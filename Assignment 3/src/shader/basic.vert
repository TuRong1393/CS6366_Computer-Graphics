#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;


smooth out vec3 smoothNormal;
flat out vec3 flatNormal;

out VS_OUT {
    vec3 fragPos;
    vec2 texCoord;
    mat3 TBN;
} vs_out;

uniform mat4 MVP;

void main(){
    gl_Position = MVP * vec4(aPos, 1.0);

    smoothNormal = aNormal;
    flatNormal = aNormal;

    vs_out.fragPos = aPos;
    vs_out.texCoord = aTexCoord;
    // Since model matrix is I, no need to calculate transpose and inverse
    // Since aTangent and aBitangent are already normalized, so here no need to normalize again
    vs_out.TBN = mat3(aTangent, aBitangent, normalize(aNormal));
}