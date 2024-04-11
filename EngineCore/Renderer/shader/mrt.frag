#version 450
#include "camera.glsli"
#include "object.glsli"
#include "shader_common.glsli"
#include "standard_vertex_output.glsli"

layout (perMaterial, binding = 0) uniform sampler2D samplerMetallicRoughness;
layout (perMaterial, binding = 1) uniform sampler2D samplerBaseColor;
layout (perMaterial, binding = 2) uniform sampler2D samplerNormalMap;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outMaterial;

void main() 
{
	// Calculate normal in tangent space
	vec3 N = normalize(inNormal);
	vec3 T = normalize(inTangent);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);
	vec3 tnorm = TBN * normalize(texture(samplerNormalMap, inUV).xyz * 2.0 - vec3(1.0));
	outNormal = vec4(tnorm * 0.5 + 0.5, 1.0) ;

	outColor = texture(samplerBaseColor, inUV);
	outMaterial.xy = texture(samplerMetallicRoughness, inUV).xy;
}