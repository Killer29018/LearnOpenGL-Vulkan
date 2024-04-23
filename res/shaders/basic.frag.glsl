#version 450

layout (location = 0) in vec3 v_Colour;

layout (location = 0) out vec4 f_Colour;

void main()
{
    f_Colour = vec4(v_Colour, 1.0);
}
