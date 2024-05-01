#version 460
#extension GL_EXT_buffer_reference : enable
#extension GL_GOOGLE_include_directive : require

layout (location = 0) out vec3 v_CameraPos;
layout (location = 1) out vec2 v_Texcoords;

#include "vertex.glsl"
// #include "object.glsl"

void main()
{
    // Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    // ObjectData data = u_Models.objects[gl_InstanceIndex];
    // mat4 model = data.model;

    // gl_Position = PushConstants.proj * PushConstants.view * model * vec4(v.position, 1.0);

    const vec3 positions[] =
    {
        vec3(-1.0, -1.0, 1.0),
        vec3(1.0, -1.0, 1.0),
        vec3(-1.0, 1.0, 1.0),
        vec3(1.0, -1.0, 1.0),
        vec3(1.0, 1.0, 1.0),
        vec3(-1.0, 1.0, 1.0),
    };

    const vec2 texCoords[] = {
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0)
    };

    gl_Position = vec4(positions[gl_VertexIndex], 1.0);

    v_CameraPos = PushConstants.cameraPos;
    v_Texcoords = texCoords[gl_VertexIndex];
}
