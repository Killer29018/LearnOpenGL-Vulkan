struct LightData
{
    vec3 position;
    vec4 colour;
    mat4 model;
};

layout (std430, set=0, binding=0) buffer readonly Lights
{
    int lightCount;
    vec4 ambient;
    LightData lights[];
} u_Lights;
