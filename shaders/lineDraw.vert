#version 450

layout(location = 0) in vec3 vertPosition;
layout(location = 1) in vec3 vertColor;
layout(location = 2) in vec3 vertOffset;

layout(location = 0) out vec3 vertColorOut;

void main()
{
    gl_Position = vec4(vertPosition + vertOffset, 1.0f);
    vertColorOut = vertColor;
}