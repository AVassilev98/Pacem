// we will be using glsl version 4.5 syntax
#version 450

layout(location = 0) in vec3 vertPosition;
layout(location = 1) in vec3 vertNormal;
layout(location = 2) in vec3 vertColor;

layout(location = 0) out vec3 vertColorOut;

const vec3 lightLoc = vec3(1.0f, -4.0f, 0.0f);

layout(push_constant) uniform constants
{
    mat4 MVP;
}
PushConstants;

void main()
{
    vec4 newPos = PushConstants.MVP * vec4(vertPosition, 1.0f);
    vec3 newNorm = mat3(PushConstants.MVP) * vertNormal;

    vec3 distToLight = normalize(vec3(newPos) - lightLoc);
    vec3 lightDir = normalize(distToLight);
    float distanceFactor = 2.0f / dot(distToLight, distToLight);
    float diffuseComponent = max(0.0f, dot(newNorm, lightDir));

    vec3 reflectedLightDir = reflect(lightDir, newNorm);
    float specularFactor = pow(max(0.0f, dot(reflectedLightDir, vec3(0.0f, 0.0f, -1.0f))), 7);
    float ambientComponent = 0.2f;

    float total = (ambientComponent + diffuseComponent + specularFactor) * distanceFactor;

    // output the position of each vertex
    gl_Position = newPos;
    vertColorOut = vertColor * total;
}
