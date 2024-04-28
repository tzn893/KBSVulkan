#ifndef GBUFFER_SET
#define GBUFFER_SET perDraw
#endif

layout (GBUFFER_SET, binding = 0) uniform sampler2D gbufferDepth;
layout (GBUFFER_SET, binding = 1) uniform sampler2D gbufferColor;
layout (GBUFFER_SET, binding = 2) uniform sampler2D gbufferNormal;
layout (GBUFFER_SET, binding = 3) uniform sampler2D gbufferMaterial;

struct GBufferData
{
    vec3 positionWS;
    vec3 normalWS;
    vec3 albedo;    
    float roughness;
    float metallic;
};


GBufferData unpackGBuffer(vec2 uv, mat4 projInverse, mat4 viewInverse)
{
    float depth = texture(gbufferDepth, uv).r;
    vec3  normal = normalize(texture(gbufferNormal, uv).xyz * 2.0 - 1.0);
    vec3  albedo = texture(gbufferColor, uv).xyz;
    vec2  metallicRoughness = texture(gbufferMaterial, uv).xy;

    vec3  ndc = vec3(uv * 2.0 - 1.0, depth);
    vec4  clip = vec4(ndc, 1.0);

    vec4 view = projInverse * clip;

    vec4 pixelPosition = viewInverse * view;
    pixelPosition /= pixelPosition.w;

    GBufferData data;
    data.positionWS = pixelPosition.xyz;
    data.normalWS = normal;
    data.albedo = albedo;
    data.roughness = metallicRoughness.y;
    data.metallic = metallicRoughness.x;

    return data;
}