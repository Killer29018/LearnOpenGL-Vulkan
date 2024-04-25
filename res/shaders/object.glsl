struct ObjectData
{
    vec4 colour;
    mat4 model;
    mat4 rotation;
};

layout (std430, binding=2) buffer readonly Model
{
    ObjectData objects[];
} u_Models;
