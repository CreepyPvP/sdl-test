#version 450

layout(location = 0) in vec3 in_position;

layout(binding = 0, set = 0) uniform GlobalUniform 
{
    mat4 projection;
    mat4 view;
} global;

void main()
{
    gl_Position = global.projection * global.view * vec4(in_position, 1.0);
}
