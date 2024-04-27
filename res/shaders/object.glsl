struct ObjectData
{
    int materialIndex;
    vec4 colour;
    mat4 model;
    mat4 rotation;
};

layout (std430, set=1, binding=2) buffer readonly Model
{
    ObjectData objects[];
} u_Models;
