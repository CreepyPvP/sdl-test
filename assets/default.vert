#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

// SDL3 uses descriptor set 1 for all uniform buffers
// Was fun figuring that out...
layout(binding = 0, set = 1) uniform GlobalUniform 
{
    mat4 projection;
    mat4 view;
} global;

layout(location = 0) out vec3 normal;

void main()
{
    normal = normalize(in_normal);
    gl_Position = global.projection * global.view * vec4(in_position, 1.0);
}
