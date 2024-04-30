#version 450
#extension GL_EXT_buffer_reference : enable
#extension GL_GOOGLE_include_directive : require

#include "object.glsl"
#include "light.glsl"

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

layout (std430, push_constant) uniform constants
{
    VertexBuffer vertexBuffer;
    ivec2 current;
} PushConstants;

layout (location = 0) out flat int v_CurrentLight;
layout (location = 1) out flat int v_ViewIndex;

void main()
{
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    ObjectData modelData = u_Models.objects[gl_InstanceIndex];
    LightData lightData = u_Lights.lights[PushConstants.current.x];

    gl_Position = lightData.proj * lightData.view[PushConstants.current.y] * modelData.model * vec4(v.position, 1.0);

    v_CurrentLight = PushConstants.current.x;
    v_ViewIndex = PushConstants.current.y;
}