#version 460

layout (location = 0) in vec2 v_texcoord;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec3 v_fragment_position;

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) uniform sampler2D diffuse_map;
layout (set = 2, binding = 1) uniform sampler2D specular_map;
layout (set = 2, binding = 2) uniform sampler2D shininess_map;


struct Light {
    vec3 position;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    vec3 constant_linear_quadratic;
};

layout(std140, set = 3, binding = 0) uniform UniformBlock {
    vec3 camera_position;
    Light lights[10];
    int number_of_lights;
};

vec3 calc_point_light(Light light, vec3 normal, vec3 fragment_position, vec3 view_direction);

void main()
{
    vec3 norm = normalize(v_normal);
    vec3 view_direction = normalize(camera_position - v_fragment_position);

    vec3 result = vec3(0.0, 0.0, 0.0);
    for(int i = 0; i < number_of_lights; i++)
        result += calc_point_light(lights[i], norm, v_fragment_position, view_direction);
    
    FragColor = vec4(result, 1.0);
}

vec3 calc_point_light(Light light, vec3 normal, vec3 fragment_position, vec3 view_direction)
{
    vec3 light_direction = normalize(light.position - fragment_position);

    // diffuse shading
    float diff = max(dot(normal, light_direction), 0.0);

    // specular shading
    vec3 reflect_direction = reflect(-light_direction, normal);
    float spec = pow(max(dot(view_direction, reflect_direction), 0.0), texture(shininess_map, v_texcoord).r * 255);

    // attenuation
    float distance    = length(light.position - fragment_position);
    float attenuation = 1.0 / (light.constant_linear_quadratic.r + light.constant_linear_quadratic.g * distance + 
  			     light.constant_linear_quadratic.b * (distance * distance));    

    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(diffuse_map, v_texcoord));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(diffuse_map, v_texcoord));
    vec3 specular = light.specular * spec * vec3(texture(specular_map, v_texcoord));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
} 