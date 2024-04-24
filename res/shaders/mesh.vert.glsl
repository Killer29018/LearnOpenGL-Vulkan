#version 460
#extension GL_EXT_buffer_reference : enable

layout (location = 0) out vec2 v_UV;

struct Vertex
{
    vec3 position;
    vec2 UV;
    // vec4 colour;
};

struct ObjectData
{
    mat4 model;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

layout (push_constant) uniform constants
{
    mat4 view;
    mat4 proj;
    VertexBuffer vertexBuffer;
} PushConstants;

layout (std430, binding=2) buffer readonly Model
{
    ObjectData objects[];
} u_Models;

void main()
{
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    ObjectData data = u_Models.objects[gl_InstanceIndex];
    mat4 model = data.model;

    gl_Position = PushConstants.proj * PushConstants.view * model * vec4(v.position, 1.0);

    v_UV = v.UV;
}
