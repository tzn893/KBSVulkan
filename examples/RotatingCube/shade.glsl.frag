#version 450

#include "shader_common.glsli"
#include "object.glsli"
#include "camera.glsli"

#include "standard_vertex_output.glsli"

layout (location = 0) out vec4 oColor;

layout (perMaterial, binding = 0) uniform MaterialUBO
{
	float time;
    vec3  baseColor;
} mat;

void main()
{
    oColor = vec4(vec3(0.5, 0.7, 1.0) * mat.time + mat.baseColor, 1);
}                   