#version 330 core

out vec4 fragColor;

in VS_OUT {
    vec3 fragPos;
    vec2 texCoord;
    mat3 TBN;
} fs_in;

smooth in vec3 smoothNormal;
flat in vec3 flatNormal;

uniform bool isSmooth;

uniform vec3 colObject;

uniform float shininess;

uniform vec3 colDirectionLightAmbient;
uniform vec3 colDirectionLightDiffuse;
uniform vec3 colDirectionLightSpecular;

uniform vec3 colPointLightAmbient;
uniform vec3 colPointLightDiffuse;
uniform vec3 colPointLightSpecular;

uniform vec3 pointLightLoc;
uniform vec3 camPos;

uniform vec3 inputDirectionLightDir;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;

uniform bool bTextureStatus;
uniform bool bNormalMapStatus;

void main(){

    // Set normal
    vec3 normal = isSmooth ? smoothNormal : flatNormal;
    normal = normalize(normal);

    if(bNormalMapStatus) {
        normal = texture(normalMap, fs_in.texCoord).rgb;
        normal = normal * 2.0 - 1.0;   
        normal = normalize(fs_in.TBN * normal); 
    }

    vec3 directionLightDir = normalize(-inputDirectionLightDir);

    vec3 pointLightDir = normalize(pointLightLoc - fs_in.fragPos);  

    // Ambient
    float ambientStrength = 0.1f;
    vec3 ambient = (colDirectionLightAmbient + colPointLightAmbient) * ambientStrength;

    // Diffuse
    vec3 directionDiffuse = max(dot(normal, directionLightDir), 0.0f) * colDirectionLightDiffuse;
	vec3 pointDiffuse = max(dot(normal, pointLightDir), 0.0f) * colPointLightDiffuse;
	vec3 diffuse = directionDiffuse + pointDiffuse;

    // Specular
    float specularStrength = 0.5f;
    vec3 camDir = normalize(camPos - fs_in.fragPos);
    // Direction
    vec3 direction_h = normalize(directionLightDir + camDir);
    vec3 directionSpecular = pow(dot(direction_h, normal), shininess) * colDirectionLightSpecular;
    // Point
    vec3 point_h = normalize(pointLightDir + camDir);
    vec3 pointSpecular = pow(dot(point_h, normal), shininess) * colPointLightSpecular;
    // direction + Point
    vec3 specular = specularStrength * (directionSpecular + pointSpecular);  
    
    fragColor = vec4((ambient + diffuse + specular) * colObject, 1.0f);
    if(bTextureStatus) {
        fragColor = texture(diffuseMap, fs_in.texCoord) * fragColor;
    }
}