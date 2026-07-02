#version 330 core

in vec3 vFragPos;
in vec3 vNormal;

out vec4 FragColor;

uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform vec3 uObjectColor;
uniform vec3 uLightColor;

void main() {
    // Ambient
    float ambientStrength = 0.25;
    vec3 ambient = ambientStrength * uLightColor;

    // Diffuse
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;

    // Specular (Blinn-Phong)
    float specularStrength = 0.4;
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * uLightColor;

    vec3 result = (ambient + diffuse + specular) * uObjectColor;
    FragColor = vec4(result, 1.0);
}
