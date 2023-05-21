// glsl version 4.5
#version 450

layout(location = 0) in vec3 vertColor;
layout(location = 1) in vec3 vertNormal;
layout(location = 2) in vec2 vertTexCoord;
layout(location = 3) in vec3 vertPosition;
layout(location = 4) in vec3 lightPos;
layout(location = 5) in mat3 TBN;

layout(set = 2, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 2, binding = 1) uniform sampler2D aoTexture;
layout(set = 2, binding = 2) uniform sampler2D emissiveTexture;
layout(set = 2, binding = 3) uniform sampler2D normalMap;

// output write
layout(location = 0) out vec4 outFragColor;

void main()
{
    vec3 norm = TBN * normalize(texture(normalMap, vertTexCoord).xyz * 2.0f - 1.0f);
    vec3 distToLight = normalize(vertPosition - lightPos);
    vec3 lightDir = normalize(distToLight);
    float distanceFactor = 0.2f / dot(distToLight, distToLight);
    float diffuseComponent = max(0.0f, dot(norm, lightDir));

    vec3 reflectedLightDir = reflect(lightDir, norm);
    float specularFactor = pow(max(0.0f, dot(reflectedLightDir, vec3(0.0f, 0.0f, -1.0f))), 7);
    float ambientComponent = 0.0f;

    float total = (ambientComponent + diffuseComponent + specularFactor) * distanceFactor;

    vec3 texColor = texture(diffuseTexture, vertTexCoord).xyz * total;
    vec3 occludedColor = texColor * texture(aoTexture, vertTexCoord).xyz;
    vec3 emissiveColor = occludedColor + texture(emissiveTexture, vertTexCoord).xyz;

    outFragColor = vec4(emissiveColor, 1.0);
}
