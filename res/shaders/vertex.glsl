struct Vertex
{
    vec3 position;
    vec2 UV;
    vec3 normal;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

layout (push_constant) uniform constants
{
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
    VertexBuffer vertexBuffer;
} PushConstants;
