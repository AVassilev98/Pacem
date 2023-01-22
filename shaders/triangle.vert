// we will be using glsl version 4.5 syntax
#version 450

layout(location = 0) in vec3 vertPosition;
layout(location = 1) in vec3 vertNormal;
layout(location = 2) in vec3 vertColor;

layout(location = 0) out vec3 vertColorOut;

const vec3 lightLoc = vec3(0.0f, 1.0f, 0.0f);
const vec3 translation = vec3(0.0f, 0.0f, -5.5f);
const float S = 1 / tan(radians(45));

const mat4 projectionMat = mat4(S, 0.0, 0.0, 0.0,                                                 // 1. column
                                0.0, S, 0.0, 0.0,                                                 // 2. column
                                0.0, 0.0, -100.0 / (100.0 - 0.5), -(100.0 * 0.5) / (100.0 - 0.5), // 3. column
                                0.0, 0.0, -1.0, 0.0);                                             // 4. column

void main()
{
    //

    // output the position of each vertex
    gl_Position = vec4((vertPosition + translation), 1.0f) * projectionMat;
    vertColorOut = vertColor;
}
