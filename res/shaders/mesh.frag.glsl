#version 450
#extension GL_GOOGLE_include_directive : require

#include "light.glsl"
#include "material.glsl"

layout (location = 0) in vec2 v_UV;
layout (location = 1) in vec4 v_Colour;
layout (location = 2) in vec3 v_Normal;
layout (location = 3) in vec4 v_FragPos;
layout (location = 4) in vec3 v_CameraPos;
layout (location = 5) in flat int v_MaterialIndex;

layout (location = 0) out vec4 f_Colour;

layout(set=1, binding = 0) uniform sampler2D u_BoxSampler;
layout(set=1, binding = 1) uniform sampler2D u_FaceSampler;

vec3 gammaCorrect(vec3 colour)
{
    return pow(colour, vec3(1./2.2));
}

vec4 invGamma(vec4 colour)
{
    return vec4(pow(colour.rgb, vec3(2.2)), colour.a);
}

vec4 v;

bool inShadow(LightData light, int layer, vec3 normal, vec3 lightDir)
{
    ivec3 size = imageSize(u_ShadowMaps);
    for (int i = 0; i < 6; i++)
    {
        vec4 position = light.proj * light.view[i] * vec4(v_FragPos.xyzw);

        vec3 projected = position.xyz / position.w; // [-1,1]
        v = vec4(projected, 1.0);
        float current = projected.z;
        projected = projected * 0.5 + 0.5; // [0,1]

        if (projected.x < 0.0 || projected.y < 0.0 || projected.x > 1.0 || projected.y > 1.0)
            continue;


        float closest = imageLoad(u_ShadowMaps, ivec3(projected.xy * size.xy, layer * 6 + i)).r;

        float bias = max(0.05 * 1.0 - dot(normal, lightDir), 0.005);
        // bias = 0.0005;

        bool inShadow = ((current - bias) > closest) ? true : false;


        return inShadow;
    }

    return false;
}

void main()
{
    vec4 box = invGamma(texture(u_BoxSampler, v_UV));
    vec4 face = invGamma(texture(u_FaceSampler, v_UV));

    MaterialData material = u_Materials.materials[v_MaterialIndex];

    vec3 colour = vec3(0);
    vec3 ambient = u_Lights.ambient.rgb * material.ambient;

    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    vec3 norm = normalize(v_Normal);
    for (int i = 0; i < u_Lights.lightCount; i++)
    {
        LightData light = u_Lights.lights[i];
        vec3 lightPos = light.position;

        float distance = length(lightPos - v_FragPos.xyz);
        float attenuation = 1.0 / (light.attenuation.x + light.attenuation.y * distance + light.attenuation.z * distance * distance);

        vec3 lightDir = normalize(lightPos - v_FragPos.xyz);
        vec3 viewDir = normalize(v_CameraPos - v_FragPos.xyz);
        vec3 halfwayDir = normalize(lightDir + viewDir);

        float diff = max(dot(norm, lightDir), 0.0);

        bool shadow = inShadow(light, i, norm, lightDir);
        // v = vec3(shadow);

        if (!shadow)
        {
            diffuse += light.diffuse * diff;
            diffuse *= attenuation;
        }

        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, halfwayDir), 0.0), material.specular.a);

        if (!shadow)
        {
            specular += light.specular * spec;
            specular *= attenuation;
        }
    }

    colour += ambient * material.ambient;
    colour += diffuse * material.diffuse;
    colour += specular * material.specular.rgb;

    if (v_MaterialIndex == 0)
        colour *= mix(box, face, 0.5).rgb;

    // colour = v;

    // f_Colour = mix(box, face, 0.5);
    // f_Colour = vec4(gammaCorrect(colour), 1.0);
    f_Colour = v;
}
