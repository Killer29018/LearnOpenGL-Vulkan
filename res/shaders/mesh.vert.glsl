#version 450
#extension GL_EXT_buffer_reference : enable

layout (location = 0) out vec2 v_UV;

struct Vertex
{
    vec3 position;
    vec2 UV;
    // vec4 colour;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

layout (push_constant) uniform constants
{
    mat4 model;
    mat4 view;
    mat4 proj;
    VertexBuffer vertexBuffer;
} PushConstants;

void main()
{
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    gl_Position = PushConstants.proj * PushConstants.view * PushConstants.model * vec4(v.position, 1.0);

    v_UV = v.UV;
}
