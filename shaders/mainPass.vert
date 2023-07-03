// we will be using glsl version 4.5 syntax
#version 450

layout(location = 0) in vec3 vertPosition;
layout(location = 1) in vec3 vertNormal;
layout(location = 2) in vec3 vertColor;
layout(location = 3) in vec2 texCoord;

layout(push_constant) uniform constants
{
    mat4 model;
    mat4 view;
    mat4 projection;
}
PushConstants;

layout(location = 0) out vec3 vertColorOut;
layout(location = 1) out vec3 vertNormalOut;
layout(location = 2) out vec2 texCoordOut;
layout(location = 3) out vec3 vertPositionOut;

void main()
{

    // output the position of each vertex
    mat4 MVP = PushConstants.projection * PushConstants.view * PushConstants.model;
    gl_Position = MVP * vec4(vertPosition, 1.0f);
    vertColorOut = vertColor;
    vertNormalOut = vertNormal;
    texCoordOut = texCoord;
    vertPositionOut = vertPosition;
}
