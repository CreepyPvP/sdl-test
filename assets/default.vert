#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(binding = 0, set = 0) uniform sampler2D noise;

// SDL3 uses descriptor set 1 for all uniform buffers
// Was fun figuring that out...
layout(binding = 0, set = 1) uniform GlobalUniform 
{
    mat4 projection;
    mat4 view;
    float time;
} global;

layout(location = 0) out vec3 out_world_pos;
layout(location = 1) out vec2 out_uv;

float NoiseLayer(vec3 world_pos, float value)
{
    vec2 noise_position = (world_pos.xz * 0.1 + vec2(global.time * 0.01)) * value;
    float noise_sample = texture(noise, noise_position).r * 2 - 1;
    return 0.2 / value * noise_sample;
}

void main()
{
    vec3 world_pos = in_position;

    float y_offset = 0;
    y_offset += NoiseLayer(world_pos, 1);
    y_offset += NoiseLayer(world_pos, 2);
    y_offset += NoiseLayer(world_pos, 4);
    y_offset += NoiseLayer(world_pos, 8);

    gl_Position = global.projection * global.view * vec4(in_position + vec3(0, y_offset, 0), 1);

    out_uv = in_uv;
    out_world_pos = vec3(world_pos.x, world_pos.y + y_offset, world_pos.z);
}
