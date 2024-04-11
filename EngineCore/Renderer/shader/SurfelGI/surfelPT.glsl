#ifndef COMMON_GLSL
#define COMMON_GLSL

#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "random.glsl"

struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec2 uv;
    vec3 tangent;
};


struct ObjDesc
{
    uint64_t vertexBufferAddr;
    uint64_t indiceBufferAddr;
    uint64_t materialSetIndex;
};

struct MaterialSet
{
    int baseColorTexIndex;
    int normalTexIndex;
    int __padding1;
    int __padding2;

    vec4 baseColor;
};

// clang-format off
struct hitPayload
{
    vec3  radiance;
    vec3 attenuation;
};

const float pi = 3.1415926;

struct GlobalUniform
{
    vec3    cameraPosition;
    int     currentFrame;
};


vec4 sampleConsineDistribution(vec2 uv, vec3 normal, float seed) 
{
  vec2 rand2 = rand_2_2(uv, seed);
  float sqrt_eta_1 = sqrt(rand2.x);
  float pi_2_eta_2 = 2 * pi * rand2.y;

  float x = sqrt_eta_1 * cos(pi_2_eta_2);
  float y = sqrt_eta_1 * sin(pi_2_eta_2);
  float z = sqrt(1 - rand2.x);

  float weight = y / pi;

  vec3 c1 = cross(normal, vec3(0.0, 0.0, 1.0));
  vec3 c2 = cross(normal, vec3(0.0, 1.0, 0.0));
  vec3 tangent, bitangent;
  if (length(c1)>length(c2))
  {
      tangent = c1;
  }
  else
  {
      tangent = c2;
  }

  tangent = normalize(tangent);
  bitangent = normalize(cross(normal, tangent));

  //rv = n * rv.y + b * rv.z + t * rv.x;
  //return vec4(normal * y + bitangent * z + tangent * x, weight);
  return vec4(normal * z + bitangent * y + tangent * x, z);
}


#endif