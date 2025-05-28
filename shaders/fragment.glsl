#version 460

layout (location = 0) in vec2 v_texcoord;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec3 v_fragmentPosition;

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) uniform sampler2D diffuse;

layout(std140, set = 3, binding = 0) uniform UniformBlock {
    vec3 lightColor;
    vec3 lightPosition;
    vec3 cameraPosition;
};



void main()
{
    vec3 objectColor = vec3(texture(diffuse, v_texcoord));

    vec3 normal = normalize(v_normal);
    vec3 lightDirection = normalize(lightPosition - v_fragmentPosition); 
    float diff = max(dot(normal, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor;

    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    float specularStrength = 0.5;
    vec3 viewDir = normalize(cameraPosition - v_fragmentPosition);
    vec3 reflectDir = reflect(-lightDirection, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor; 
    
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}