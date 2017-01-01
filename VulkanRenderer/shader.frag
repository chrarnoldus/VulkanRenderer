#version 450

const vec3 lightPosition = vec3(0.0, 0.0, 0.0);
const float shininess = 32.0;
const float specularCoeff = 0.05;
const float diffuseCoeff = 0.7;
const float ambientCoeff = 0.3;

in vec3 position;
in vec3 normal;
in vec3 color;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 normalDir = normalize(normal);
    vec3 lightDir = normalize(lightPosition - position);
    vec3 viewDir = normalize(-position);
    vec3 reflectDir = reflect(-lightDir, normalDir);

    vec3 specular = vec3(specularCoeff * pow(max(dot(reflectDir, viewDir), 0.0), shininess));
    vec3 diffuse = diffuseCoeff * color * max(dot(normalDir, lightDir), 0.0);
    vec3 ambient = ambientCoeff * color;

    fragColor = vec4(specular + diffuse + ambient, 1.0);
}
