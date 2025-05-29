#version 460

layout (location = 0) in vec2 v_texcoord;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec3 v_fragmentPosition;

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) uniform sampler2D diffuseMap;
layout (set = 2, binding = 1) uniform sampler2D specularMap;
layout (set = 2, binding = 2) uniform sampler2D shininessMap;


struct Light {
    vec3 position;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    vec3 constantlinearQuadratic;
};

layout(std140, set = 3, binding = 0) uniform UniformBlock {
    vec3 cameraPosition;
    Light lights[10];
    int numberOfLights;
};

vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    vec3 norm = normalize(v_normal);
    vec3 viewDir = normalize(cameraPosition - v_fragmentPosition);

    vec3 result = vec3(0.0, 0.0, 0.0);
    for(int i = 0; i < numberOfLights; i++)
        result += CalcPointLight(lights[i], norm, v_fragmentPosition, viewDir);
    
    FragColor = vec4(result, 1.0);
}

vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), texture(shininessMap, v_texcoord).r * 255);
    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constantlinearQuadratic.r + light.constantlinearQuadratic.g * distance + 
  			     light.constantlinearQuadratic.b * (distance * distance));    
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(diffuseMap, v_texcoord));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(diffuseMap, v_texcoord));
    vec3 specular = light.specular * spec * vec3(texture(specularMap, v_texcoord));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
} 