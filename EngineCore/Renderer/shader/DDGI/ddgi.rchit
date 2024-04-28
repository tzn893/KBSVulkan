/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_debug_printf : enable

#include "surfelPT.glsl"


/*
RayTracingKHR

SPV_KHR_ray_tracing

5339

HitAttributeKHR
Used for storing attributes of geometry intersected by a ray. 
Visible across all functions in the current invocation. Not 
shared externally. Variables declared with this storage class 
are allowed only in IntersectionKHR, AnyHitKHR and ClosestHitKHR 
execution models. They can be written to only in IntersectionKHR 
execution model and read from only in AnyHitKHR and ClosestHitKHR 
execution models. They cannot have initializers.
*/




hitAttributeEXT vec2 attribs;
layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 1) rayPayloadEXT   bool       isShadowed;

layout(buffer_reference, scalar) buffer Indices {ivec3 i[]; }; // Triangle indices
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; }; // Positions of an object
layout(set = 0, binding = 2, scalar) buffer ObjDesc_ { ObjDesc i[]; } objDesc;
layout(set = 0, binding = 1) buffer LightBuffer
{
  Light Lights[];
};

layout(set = 0, binding = 3) buffer _MaterialSet
{
  Material materialSets[];
};

layout(set = 0, binding = 4) uniform _GlobalUniform
{
  SurfelUniform ubo;
};

layout(set = 0, binding = 0) uniform accelerationStructureEXT TLAS;

// bindless diffuse color
layout(set = 1, binding = 0) uniform sampler2D rtTextures[];

// clang-format on

#include "../PathTracing/Common/Random.glsl"
#include "../PathTracing/Common/Math.glsl"

#include "../PathTracing/Common/Sampling.glsl"
#include "../PathTracing/BSDFs/UE4BSDF.glsl"
#include "../PathTracing/BSDFs/DisneyBSDF.glsl"

#include "../PathTracing/Common/DirectLight.glsl"

void main()
{
  
  
  // Object data
  ObjDesc    objResource = objDesc.i[gl_InstanceCustomIndexEXT];
  Indices    indices     = Indices(objResource.indiceBufferAddr);
  Vertices   vertices    = Vertices(objResource.vertexBufferAddr);

  ivec3 ind = indices.i[gl_PrimitiveID + objResource.indexPrimitiveOffset];

  // Vertex of the triangle
  Vertex v0 = vertices.v[ind.x];
  Vertex v1 = vertices.v[ind.y];
  Vertex v2 = vertices.v[ind.z];

  vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

  
  // Computing the coordinates of the hit position
  const vec3 pos      = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
  const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));  // Transforming the position to world space
  
  // Computing the normal at hit position
  const vec3 nrm      = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
  const vec3 worldNrm = normalize(vec3(nrm * gl_WorldToObjectEXT));  // Transforming the normal to world space

  const vec2 uv = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;

  Material material;
  material.albedo = materialSets[int(objResource.materialSetIndex)].albedo;
  material.metallic = 0.0;
  material.roughness = 1.0;
  
  uint albedoTexID = materialSets[int(objResource.materialSetIndex)].albedoTexID;
  uint normalTexIndex = materialSets[int(objResource.materialSetIndex)].normalmapTexID;

  if(albedoTexID >= 0)
  {
    int txtId = int(albedoTexID);
    material.albedo *= texture(rtTextures[txtId], uv);
  }

  vec3 tnorm = worldNrm;

  if(normalTexIndex >= 0)
  {
    const vec3 tangent  = v0.tangent * barycentrics.x + v1.tangent * barycentrics.y + v2.tangent * barycentrics.z;
    const vec3 worldTangent = normalize((gl_ObjectToWorldEXT * vec4(tangent, 0.0)).xyz);

    vec3 bitangent = cross(worldNrm, worldTangent);

    int txtId = int(normalTexIndex);
    mat3 TBN = mat3(worldTangent, bitangent, worldNrm);
	  
    tnorm = TBN * normalize(texture(rtTextures[txtId], uv).xyz * 2.0 - vec3(1.0));
  }

  material.metallic = 0.0;
  material.roughness = 1.0;

  seed = tea(gl_LaunchIDEXT.y * gl_LaunchIDEXT.x + gl_LaunchIDEXT.x, ubo.frame);
  uint lightIdx = lcg(seed) % ubo.lights;
  Light light = Lights[lightIdx];

  vec3 ffnormal = tnorm;

  // nee light estimation
  // Tracing shadow ray only if the light is visible from the surface

  #include "../PathTracing/Raytracer/Integrators/PathTracer.glsl"
}
