#version 460
#extension GL_EXT_buffer_reference : enable
#extension GL_GOOGLE_include_directive : require

#include "vertex.glsl"
#include "light.glsl"

layout (location = 0) out vec4 v_Colour;

void main()
{
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    LightData data = u_Lights.lights[gl_InstanceIndex];
    mat4 model = data.model;

    gl_Position = PushConstants.proj * PushConstants.view * model * vec4(v.position, 1.0);

    v_Colour = data.colour;
}
