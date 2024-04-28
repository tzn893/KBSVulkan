#extension GL_EXT_debug_printf : enable
#extension GL_EXT_ray_tracing : require

#include "surfelPT.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;

layout(set = 0, binding = 1) buffer LightBuffer
{
  Light Lights[];
};

layout(set = 0, binding = 4) uniform _GlobalUniform
{
  SurfelUniform ubo;
};

#include "../PathTracing/Common/Random.glsl"
#include "../PathTracing/Common/Math.glsl"
#include "../PathTracing/Common/Sampling.glsl"

void main()
{
    // Stop path tracing loop from rgen shader
	payload.stop = true;
	payload.ffnormal = vec3(0.);
	payload.worldPos = vec3(0.);
	

	LightSample lightSample;

	vec3 Le = vec3(0.0);
	if (interesetsEmitter(lightSample, INFINITY))
	{
		Le = sampleEmitter(lightSample, payload.bsdf);
	}
	else
	{
		Le = sampleInifinitEmitter();
	}

	payload.radiance += Le * payload.beta;
	return;
}