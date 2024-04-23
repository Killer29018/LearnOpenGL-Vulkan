#version 450
#extension GL_EXT_buffer_reference : enable

layout (location = 0) out vec3 v_Colour;
layout (location = 1) out vec2 v_UV;

struct Vertex
{
    vec3 position;
    vec2 UV;
    vec3 colour;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

layout (push_constant) uniform constants
{
    mat4 view;
    mat4 proj;
    mat4 model;
    VertexBuffer vertexBuffer;
} PushConstants;

void main()
{
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    gl_Position = vec4(v.position, 1.0);

    v_Colour = v.colour;
}
