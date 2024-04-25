#version 460
#extension GL_EXT_buffer_reference : enable
#extension GL_GOOGLE_include_directive : require

#include "vertex.glsl"
#include "object.glsl"

layout (location = 0) out vec2 v_UV;
layout (location = 1) out vec4 v_Colour;
layout (location = 2) out vec3 v_Normal;
layout (location = 3) out vec3 v_FragPos;
layout (location = 4) out vec3 v_CameraPos;

void main()
{
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    ObjectData data = u_Models.objects[gl_InstanceIndex];
    mat4 model = data.model;

    gl_Position = PushConstants.proj * PushConstants.view * model * vec4(v.position, 1.0);

    v_UV = v.UV;
    v_Colour = data.colour;
    v_Normal = vec3(data.rotation * vec4(v.normal, 1.0));
    v_FragPos = vec3(model * vec4(v.position, 1.0));

    v_CameraPos = PushConstants.cameraPos;
}
