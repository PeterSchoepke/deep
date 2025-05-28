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

    vec3 materialAmbient = objectColor * 0.1;
    vec3 materialDiffuse = objectColor;
    vec3 materialSpecular = objectColor * 1.5;
    float materialShininess = 32;

    // ambient
    vec3 ambient = lightColor * materialAmbient;

    // diffuse 
    vec3 norm = normalize(v_normal);
    vec3 lightDir = normalize(lightPosition - v_fragmentPosition);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = lightColor * (diff * materialDiffuse);
    
    // specular
    vec3 viewDir = normalize(cameraPosition - v_fragmentPosition);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    vec3 specular = lightColor * (spec * materialSpecular);  
        
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}