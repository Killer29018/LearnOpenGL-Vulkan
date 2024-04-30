#version 460 core
#extension GL_GOOGLE_include_directive : require

#include "light.glsl"

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

layout (location = 0) in flat int v_CurrentLight[3];

// layout (location = 0) out flat int g_Layer;
// layout (location = 1) out vec4 g_FragPos;

void main()
{
    LightData light = u_Lights.lights[v_CurrentLight[0]];

    for (int face = 0; face < 6; face++)
    {
        gl_Layer = v_CurrentLight[0] * 6 + face;
        // gl_Layer = g_Layer;
        for (int i = 0; i < 3; i++)
        {
            gl_Position = light.proj * light.view[face] * gl_in[i].gl_Position;
            // gl_Position = g_FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}
