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
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
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
layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(location = 1) rayPayloadEXT   bool       isShadowed;
layout(location = 2) rayPayloadEXT   hitPayload prd_o;

layout(buffer_reference, scalar) buffer Indices {ivec3 i[]; }; // Triangle indices
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; }; // Positions of an object
layout(set = 0, binding = 2, scalar) buffer ObjDesc_ { ObjDesc i[]; } objDesc;
layout(set = 0, binding = 1) uniform LightBuffer
{
  vec3 lightVec;
  int  lightType;
  vec3 lightIntensity;
} lightBuffer;

layout(set = 0, binding = 3) uniform _MaterialSet
{
  MaterialSet materialSets[128];
};

layout(set = 0, binding = 4) uniform _GlobalUniform
{
  GlobalUniform globalUniform;
};

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

// bindless diffuse color
layout(set = 1, binding = 0) uniform sampler2D rtTextures[];

// clang-format on


void main()
{
  debugPrintfEXT("any thing hit");

  // Object data
  ObjDesc    objResource = objDesc.i[gl_InstanceCustomIndexEXT];
  Indices    indices     = Indices(objResource.indiceBufferAddr);
  Vertices   vertices    = Vertices(objResource.vertexBufferAddr);

  ivec3 ind = indices.i[gl_PrimitiveID];

  // Vertex of the triangle
  Vertex v0 = vertices.v[ind.x];
  Vertex v1 = vertices.v[ind.y];
  Vertex v2 = vertices.v[ind.z];

  vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
  
  
  // Computing the coordinates of the hit position
  const vec3 pos      = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
  const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));  // Transforming the position to world space
  
  // Computing the normal at hit position
  const vec3 nrm      = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
  const vec3 worldNrm = normalize(vec3(nrm * gl_WorldToObjectEXT));  // Transforming the normal to world space

  const vec2 uv = v0.uv * barycentrics.x + v1.uv * barycentrics.y + v2.uv * barycentrics.z;
  // Vector toward the light
  // sample a light

  vec3  shadowDir, L;
  vec3 lightIntensity = lightBuffer.lightIntensity;
  float lightDistance  = 100000.0;
  float shadowWeight = 1e-6;

  MaterialSet mat = materialSets[int(objResource.materialSetIndex)];
  
  vec3 diffuse = vec3(1);

  if(mat.baseColorTexIndex >= 0)
  {
    int txtId    = mat.baseColorTexIndex;
    diffuse *= texture(rtTextures[nonuniformEXT(txtId)], uv).xyz;
  }

  vec3 tnorm = worldNrm;

  if(mat.normalTexIndex >= 0)
  {
    const vec3 tangent  = v0.tangent * barycentrics.x + v1.tangent * barycentrics.y + v2.tangent * barycentrics.z;
    const vec3 worldTangent = normalize((gl_ObjectToWorldEXT * vec4(tangent, 0.0)).xyz);

    vec3 bitangent = cross(worldNrm, worldTangent);

    int txtId = mat.normalTexIndex;
    mat3 TBN = mat3(worldTangent, bitangent, worldNrm);
	  
    tnorm = TBN * normalize(texture(rtTextures[nonuniformEXT(txtId)], uv).xyz * 2.0 - vec3(1.0));
  }

  
  if(lightBuffer.lightType == 0/*envoriment light*/)
  {
    // sample a random direction in semi-sphere
    vec4 shadowDirW = sampleConsineDistribution(uv, tnorm, float(globalUniform.currentFrame));
    shadowDir = shadowDirW.xyz;
    shadowWeight = max(shadowWeight, shadowDirW.w);
  }
  else if(lightBuffer.lightType == 1/*sun light*/)
  {
    shadowDir = lightBuffer.lightVec;
    shadowWeight = 1 / pi;
  }

  // Material of the object
  // Diffuse
  diffuse = dot(L, tnorm) * lightIntensity * vec3(mat.baseColor) * diffuse;

  // nee light estimation
  // Tracing shadow ray only if the light is visible from the surface
  vec3  origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
  
  if(dot(tnorm, shadowDir) > 0)
  {
    float tMin   = 0.001;
    float tMax   = lightDistance;
    vec3  rayDir = shadowDir;
    uint  flags  = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    isShadowed   = true;
    traceRayEXT(topLevelAS,  // acceleration structure
                flags,       // rayFlags
                0xFF,        // cullMask
                0,           // sbtRecordOffset
                0,           // sbtRecordStride
                1,           // missIndex
                origin,      // ray origin
                tMin,        // ray min range
                rayDir,      // ray direction
                tMax,        // ray max range
                1            // payload (location = 1)
    );
  }


  if(isShadowed)
  {
    L += prd.attenuation * lightIntensity * dot(tnorm, shadowDir) /  shadowWeight;
  }
  
  vec4 sampleDirW = sampleConsineDistribution(uv, tnorm, float(globalUniform.currentFrame) + 119);
  float sampleWeight = max(sampleDirW.w, 1e-6);

  prd_o.attenuation = dot(tnorm, sampleDirW.xyz) * prd.attenuation * diffuse /  sampleWeight;
  prd_o.radiance = vec3(0.0);

  traceRayEXT( 
    topLevelAS,
    gl_RayFlagsOpaqueEXT,
    0xff,
    0,0,
    0,
    origin,
    0.001,
    sampleDirW.xyz,
    lightDistance,
    2
  );

  L += prd_o.radiance;

  prd.radiance = L;
}
