#version 460

layout (location = 0) in vec2 v_texcoord;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec3 v_fragmentPosition;

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) uniform sampler2D diffuse;

struct Light {
    vec3 position;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

layout(std140, set = 3, binding = 0) uniform UniformBlock {
    vec3 cameraPosition;
    Light light;
};


void main()
{
    vec3 objectColor = vec3(texture(diffuse, v_texcoord));

    vec3 materialAmbient = objectColor * 0.1;
    vec3 materialDiffuse = objectColor;
    vec3 materialSpecular = objectColor * 1.5;
    float materialShininess = 32;

    // ambient
    vec3 ambient = light.ambient * materialAmbient;

    // diffuse 
    vec3 norm = normalize(v_normal);
    vec3 lightDir = normalize(light.position - v_fragmentPosition);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * materialDiffuse);
    
    // specular
    vec3 viewDir = normalize(cameraPosition - v_fragmentPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    vec3 specular = light.specular * (spec * materialSpecular);
        
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}