#version 450

#include "shader_common.glsli"
#include "object.glsli"
#include "camera.glsli"

#include "standard_vertex_output.glsli"

layout (location = 0) out vec4 oColor;

layout (perMaterial, binding = 0) uniform MaterialUBO
{
    vec3  baseColor;
} mat;

layout(perMaterial, binding = 1) uniform sampler2D albedo;

void main()
{
    vec3 albedoColor = texture(albedo, inUV).xyz;

    oColor = vec4(albedoColor, 1.0);
}