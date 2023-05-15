#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) out vec3 color;
layout(location = 1) out vec3 Normal;
layout(location = 2) out vec2 texCoordOut;
layout(location = 3) out vec3 crntPos;
layout(location = 4) out vec3 lightPos;
layout(location = 5) out mat3 TBNOut;

layout(location = 0) in vec3 vertColor[];
layout(location = 1) in vec3 vertNormal[];
layout(location = 2) in vec2 texCoord[];
layout(location = 3) in vec3 vertPosition[];

layout(push_constant) uniform constants
{
    mat4 model;
    mat4 vp;
}
PushConstants;

const vec3 lightLoc = vec3(1.0f, -4.0f, 0.0f);

void main()
{
    // Edges of the triangle
    vec3 edge0 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 edge1 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    // Lengths of UV differences
    vec2 deltaUV0 = texCoord[1] - texCoord[0];
    vec2 deltaUV1 = texCoord[2] - texCoord[0];

    // one over the determinant
    float invDet = 1.0f / (deltaUV0.x * deltaUV1.y - deltaUV1.x * deltaUV0.y);

    vec3 tangent = vec3(invDet * (deltaUV1.y * edge0 - deltaUV0.y * edge1));
    vec3 bitangent = vec3(invDet * (-deltaUV1.x * edge0 + deltaUV0.x * edge1));

    vec3 T = normalize(vec3(PushConstants.model * vec4(tangent, 0.0f)));
    vec3 B = normalize(vec3(PushConstants.model * vec4(bitangent, 0.0f)));
    vec3 N = normalize(vec3(PushConstants.model * vec4(cross(edge1, edge0), 0.0f)));

    mat3 TBN = mat3(T, B, N);
    // TBN is an orthogonal matrix and so its inverse is equal to its transpose
    TBN = transpose(TBN);

    const mat4 MVP = PushConstants.vp * PushConstants.model;

    gl_Position = MVP * gl_in[0].gl_Position;
    Normal = vec3(MVP * vec4(vertNormal[0], 1.0f));
    color = vertColor[0];
    texCoordOut = texCoord[0];
    // Change all lighting variables to TBN space
    crntPos = vec3(MVP * vec4(vertPosition[0], 1.0f));
    lightPos = lightLoc;
    TBNOut = TBN;
    EmitVertex();

    gl_Position = MVP * gl_in[1].gl_Position;
    Normal = vec3(MVP * vec4(vertNormal[1], 1.0f));
    color = vertColor[1];
    texCoordOut = texCoord[1];
    // Change all lighting variables to TBN space
    crntPos = vec3(MVP * vec4(vertPosition[1], 1.0f));
    lightPos = lightLoc;
    TBNOut = TBN;
    EmitVertex();

    gl_Position = MVP * gl_in[2].gl_Position;
    Normal = vec3(MVP * vec4(vertNormal[2], 1.0f));
    color = vertColor[2];
    texCoordOut = texCoord[2];
    // Change all lighting variables to TBN space
    crntPos = vec3(MVP * vec4(vertPosition[2], 1.0f));
    lightPos = lightLoc;
    TBNOut = TBN;
    EmitVertex();

    EndPrimitive();
}