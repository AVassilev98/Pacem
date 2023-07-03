// glsl version 4.5
#version 450

layout(location = 0) in vec3 vertColor;
layout(location = 1) in vec3 vertNormal;
layout(location = 2) in vec2 vertTexCoord;
layout(location = 3) in vec3 vertPosition;

layout(set = 2, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 2, binding = 1) uniform sampler2D aoTexture;
layout(set = 2, binding = 2) uniform sampler2D emissiveTexture;
layout(set = 2, binding = 3) uniform sampler2D normalMap;

// output write
layout(location = 0) out vec2 outNormalXY;
layout(location = 1) out vec4 outDiffuseColor;

const vec3 lightPos = vec3(1.0f, -4.0f, 0.0f);

void main()
{
    vec3 norm = vec3(normalize(texture(normalMap, vertTexCoord)));
    outDiffuseColor = vec4(texture(diffuseTexture, vertTexCoord).xyz, 1.0f);
    outNormalXY = vec2(norm.xy);
}
