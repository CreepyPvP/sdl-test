#version 450

layout(location = 0) in vec3 world_pos;
layout(location = 1) in vec2 uv;

layout(binding = 0, set = 2) uniform sampler2D noise;

layout(location = 0) out vec4 final_color;

vec3 sun_dir = normalize(vec3(1, 2, 3));
vec3 sun_color = vec3(1.2, 1.2, 1.2);

void main()
{
    vec3 pos_dx = dFdxFine(world_pos);
    vec3 pos_dy = dFdyFine(world_pos);
    vec3 n = normalize(cross(pos_dx, pos_dy));
    n.y *= -1;

    vec3 water_color = vec3(0.2, 0.3, 0.4);
    
    // float noise_sample = texture(noise, uv).r;
    // final_color = vec4(vec3(noise_sample), 1);
    // final_color = vec4(uv, 0, 1);

    vec3 light = vec3(0.1);
    light += clamp(dot(sun_dir, n), 0, 1) * sun_color; 

    final_color = vec4(water_color * light, 1);
}
