#ifndef COMMON_GLSL
#define COMMON_GLSL

#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct Vertex
{
	vec3 position;
	vec3 normal;
	vec2 texCoord;
	//int materialIndex;
	vec3 tangent;
};


struct SurfelUniform
{
	vec3 cameraPos;
	uint lights;
	bool doubleSidedLight;
	uint spp;
	uint maxDepth;
	uint frame;
};

#include "../PathTracing/Common/Structs.glsl"

#include "../PathTracing/Common/Random.glsl"

vec4 sampleConsineDistribution(vec2 uv, vec3 normal, inout uint seed) 
{
  float rx = rnd(seed);
  float ry = rnd(seed);

  float sqrt_eta_1 = sqrt(rx);
  float pi_2_eta_2 = 2 * PI * ry;

  float x = sqrt_eta_1 * cos(pi_2_eta_2);
  float y = sqrt_eta_1 * sin(pi_2_eta_2);
  float z = sqrt(1 - rx);

  float weight = y / PI;

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