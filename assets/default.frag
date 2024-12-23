#version 450

layout(location = 0) in vec3 normal;

layout(location = 0) out vec4 finalcolor;

void main()
{
    finalcolor = vec4(normal, 1);
}
