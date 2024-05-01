#version 450
#extension GL_GOOGLE_include_directive : require

#include "light.glsl"
#include "material.glsl"

layout (location = 0) in vec3 v_CameraPos;
layout (location = 1) in vec2 v_Texcoords;

layout (location = 0) out vec4 f_Colour;

layout(set=1, binding = 0) uniform sampler2D u_Position;
layout(set=1, binding = 1) uniform sampler2D u_Normal;
layout(set=1, binding = 2) uniform sampler2D u_TexData;

vec3 gammaCorrect(vec3 colour)
{
    return pow(colour, vec3(1./2.2));
}

vec4 invGamma(vec4 colour)
{
    return vec4(pow(colour.rgb, vec3(2.2)), colour.a);
}

bool inShadow(LightData light, vec4 fragPos, int layer, vec3 normal, vec3 lightDir)
{
    for (int i = 0; i < 6; i++)
    {
        vec4 position = light.proj * light.view[i] * fragPos;

        vec3 projected = position.xyz / position.w; // [-1,1]

        if (projected.z > 1.0 || projected.z < 0.0)
            continue;

        float current = projected.z;
        projected = projected * 0.5 + 0.5; // [0,1]

        if (projected.x < 0.0 || projected.y < 0.0 || projected.x > 1.0 || projected.y > 1.0)
            continue;

        float closest = texture(u_ShadowMaps, vec3(projected.xy, layer * 6 + i)).r;

        float bias = max(0.005 * 1.0 - dot(normal, lightDir), 0.0005);

        bool inShadow = ((current + bias) < closest) ? true : false;

        return inShadow;
    }

    return false;
}

void main()
{
    vec4 positionSample = texture(u_Position, v_Texcoords);
    vec4 normalSample = texture(u_Normal, v_Texcoords);
    vec4 texSample = texture(u_TexData, v_Texcoords);

    if (texSample.w < 1.0)
        discard;

    vec4 position = positionSample;
    vec3 normal = normalSample.xyz;

    vec2 uv = texSample.xy;
    int materialIndex = int(texSample.z);

    vec4 box = invGamma(texture(u_BoxSampler, uv));
    vec4 face = invGamma(texture(u_FaceSampler, uv));

    MaterialData material = u_Materials.materials[materialIndex];

    vec3 colour = vec3(0);
    vec3 ambient = u_Lights.ambient.rgb * material.ambient;

    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    vec3 norm = normalize(normal);
    for (int i = 0; i < u_Lights.lightCount; i++)
    {
        LightData light = u_Lights.lights[i];
        vec3 lightPos = light.position;

        float distance = length(lightPos - position.xyz);
        float attenuation = 1.0 / (light.attenuation.x + light.attenuation.y * distance + light.attenuation.z * distance * distance);

        vec3 lightDir = normalize(lightPos - position.xyz);
        vec3 viewDir = normalize(v_CameraPos - position.xyz);
        vec3 halfwayDir = normalize(lightDir + viewDir);

        float diff = max(dot(norm, lightDir), 0.0);

        bool shadow = inShadow(light, position, i, norm, lightDir);
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

    if (materialIndex == 0)
        colour *= mix(box, face, 0.5).rgb;

    f_Colour = vec4(gammaCorrect(colour), 1.0);
}
