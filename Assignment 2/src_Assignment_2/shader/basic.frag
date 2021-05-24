#version 330 core

out vec4 fragColor;

in vec3 fragPos;

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

void main(){

    // Set normal
    vec3 normal = isSmooth ? smoothNormal : flatNormal;
    normal = normalize(normal);

    vec3 directionLightDir = vec3(0, -1, -1);
    directionLightDir = normalize(-directionLightDir);

    vec3 pointLightDir = normalize(pointLightLoc - fragPos);  

    // Ambient
    float ambientStrength = 0.1f;
    vec3 ambient = (colDirectionLightAmbient + colPointLightAmbient) * ambientStrength;

    // Diffuse
    vec3 directionDiffuse = max(dot(normal, directionLightDir), 0.0f) * colDirectionLightDiffuse;
	vec3 pointDiffuse = max(dot(normal, pointLightDir), 0.0f) * colPointLightDiffuse;
	vec3 diffuse = directionDiffuse + pointDiffuse;

    // Specular
    float specularStrength = 0.5f;
    vec3 camDir = normalize(camPos - fragPos);
    // Direction
    vec3 direction_h = normalize(directionLightDir + camDir);
    vec3 directionSpecular = pow(dot(direction_h, normal), shininess) * colDirectionLightSpecular;
    // Point
    vec3 point_h = normalize(pointLightDir + camDir);
    vec3 pointSpecular = pow(dot(point_h, normal), shininess) * colPointLightSpecular;
    // direction + Point
    vec3 specular = specularStrength * (directionSpecular + pointSpecular);  
    

    fragColor = vec4((ambient + diffuse + specular) * colObject, 1.0f);
    //fragColor = vec4(colObject, 1.0f);
}