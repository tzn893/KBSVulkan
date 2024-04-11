
precision highp float;
precision highp int;


#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#extension GL_EXT_debug_printf : require

// Replaced by Compiler.h
#define USE_GAMMA_CORRECTION

#include "../Common/Structs.glsl"
#include "../Common/Vertex.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 1) rayPayloadEXT bool isShadowed;

layout(binding = 0, set = 0) uniform accelerationStructureEXT TLAS;
layout(binding = 3) readonly uniform UniformBufferObject { Uniform ubo; };
layout(binding = 5) readonly buffer ObjDescs {ObjDesc[] obj;};

// layout(binding = 4) readonly buffer VertexArray { float Vertices[]; };
// layout(binding = 5) readonly buffer IndexArray { uint Indices[]; };

layout(binding = 4) readonly buffer MaterialArray { Material[] Materials; };
layout(binding = 6) readonly buffer LightArray { Light[] Lights; };

layout(binding = 0, set = 1) uniform sampler2D[] TextureSamplers;
/*
#ifdef USE_HDR
layout(binding = 12) uniform sampler2D[] HDRs;
#endif
*/

hitAttributeEXT vec2 hit;

#include "../Common/Random.glsl"
#include "../Common/Math.glsl"

#ifdef USE_HDR
#include "../Common/HDR.glsl"
#endif

#include "../Common/Sampling.glsl"
#include "../BSDFs/UE4BSDF.glsl"
#include "../BSDFs/DisneyBSDF.glsl"

#include "../Common/DirectLight.glsl"
void main()
{
	ObjDesc objDesc = obj[gl_InstanceCustomIndexEXT];
	Triangle tri = unpack(objDesc);

	const Vertex v0 = tri.v[0];
	const Vertex v1 = tri.v[1];
	const Vertex v2 = tri.v[2];

	Material material = Materials[int(objDesc.materialSetIndex)];

	const vec3 barycentrics = vec3(1.0 - hit.x - hit.y, hit.x, hit.y);
	const vec2 texCoord = mix(v0.texCoord, v1.texCoord, v2.texCoord, barycentrics);
	const vec3 worldPos = mix(v0.position, v1.position, v2.position, barycentrics);
	vec3 normal = normalize(mix(v0.normal, v1.normal, v2.normal, barycentrics));
	// face forward normal
	vec3 ffnormal = dot(normal, gl_WorldRayDirectionEXT) <= 0.0 ? normal : normal * -1.0;
	float eta = dot(normal, ffnormal) > 0.0 ? (1.0 / material.ior) : material.ior;

	// Update the material properties using textures

	if (material.albedoTexID >= 0)
	{
		material.albedo.xyz *= texture(TextureSamplers[material.albedoTexID], texCoord).xyz;
	}

	if (material.metallicRoughnessTexID >= 0)
	{
		vec2 metallicRoughness = texture(TextureSamplers[material.metallicRoughnessTexID], texCoord).zy;
		material.metallic = metallicRoughness.x;
		material.roughness = metallicRoughness.y;
	}

	if (material.normalmapTexID >= 0)
	{
		// Orthonormal Basis
		mat3 frame = localFrame(ffnormal);
		vec3 nrm = texture(TextureSamplers[material.normalmapTexID], texCoord).xyz;
		nrm = frame * normalize(nrm * 2.0 - 1.0);
		normal = normalize(nrm);
		ffnormal = dot(normal, gl_WorldRayDirectionEXT) <= 0.0 ? normal : normal * -1.0;
	}

	seed = tea(gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x, ubo.frame);

	payload.worldPos = worldPos;
	payload.normal = normal;
	payload.ffnormal = ffnormal;
	payload.eta = eta;

	// payload.radiance = vec3(material.metallic, material.roughness, 0.0);

	//vec3 hit = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	//payload.radiance = ffnormal * 0.5 + 0.5;

	/*
	// Albedo
	// Metallic and Roughness
	// Normal map
	*/


	// Replaced by Compiler.h
	#include "Integrators/PathTracer.glsl"
}
