

#ifndef DDGI_GBUFFER_SET
#define DDGI_GBUFFER_SET 1
#endif

// (positionWS, albedo.x)
layout(set = DDGI_GBUFFER_SET, binding = 0, rgba16f) image2D gbuffer0;
// (normalWS, albedo.y)
layout(set = DDGI_GBUFFER_SET, binding = 1, rgba16f) image2D gbuffer1;
// (albedo.z, roughness, metallic)
layout(set = DDGI_GBUFFER_SET, binding = 2, rgba16f) image2D gbuffer2;

struct GBufferData
{
    vec3 normalWS;
    vec3 positionWS;
    vec3 albedo;
    float roughness;
    float metallic;
};

