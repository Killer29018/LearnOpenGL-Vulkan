#version 450

layout (location = 0) out vec3 v_Colour;

void main()
{
    const vec2 positions[3] = vec2[3](
        vec2(-0.5, 0.5),
        vec2(0.0, -0.5),
        vec2(0.5, 0.5)
    );

    const vec3 colours[3] = vec3[3](
        vec3(1.0, 0.0, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, 0.0, 1.0)
    );

    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    v_Colour = colours[gl_VertexIndex];
}
