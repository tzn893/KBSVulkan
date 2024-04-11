/*
 * Path tracing integral. 
 * Code excerpt executed in the Hit shader with the following available propeties:
 *
 * Available Uniforms:
 * [accelerationStructureNV TLAS, Uniform ubo, TextureSamplers[], Lights[]]
 *
 * Availavle variables:
 * [Material material, vec3 barycentrics, vec3 normal, vec2 texCoord, vec3 worldPos]
 * 
 * Available payloads:
 * [rayPayloadInNV RayPayload payload, rayPayloadNV bool isShadowed]
 */
{
	BsdfSample bsdfSample;
	LightSample lightSample;	

	payload.radiance += material.emission.xyz * payload.beta;

	/*
	if(any(isnan(payload.radiance)))
	{
		debugPrintfEXT("nan in step 1");
	}
	*/
	

	if (interesetsEmitter(lightSample, gl_HitTEXT))
	{
		vec3 Le = sampleEmitter(lightSample, payload.bsdf);

		if (ubo.doubleSidedLight || dot(payload.ffnormal, lightSample.normal) > 0.f)
			payload.radiance += Le * payload.beta;

		payload.stop = true;
		return;
	}

	
	/*
	if(any(isnan(payload.radiance)))
	{
		debugPrintfEXT("nan in step 2");
	}
	*/
	

	
	payload.beta *= exp(-payload.absorption * gl_HitTEXT);
	payload.radiance += directLight(material) * payload.beta;

	
	/*
	if(any(isnan(payload.radiance)))
	{
		debugPrintfEXT("nan in step 3");
	}
	*/

	vec3 F = DisneySample(material, bsdfSample.bsdfDir, bsdfSample.pdf);

	float cosTheta = abs(dot(ffnormal, bsdfSample.bsdfDir));

	if (bsdfSample.pdf <= 0.0)
	{
		payload.stop = true;
		return;
	}

	payload.beta *= F * cosTheta / (bsdfSample.pdf + EPS);

	
	/*
	if(any(isnan(payload.beta)))
	{
		debugPrintfEXT("nan in step 4");
	}
	*/
	

	if (dot(ffnormal, bsdfSample.bsdfDir) < 0.0)
		payload.absorption = -log(material.extinction.xyz) / (material.atDistance + EPS);

	// Russian roulette
	if (max3(payload.beta) < 0.01f && payload.depth > 2)
	{
		float q = max(float(.05), 1.f - max3(payload.beta));
		if (rnd(seed) < q)
			payload.stop = true;
		payload.beta /= (1.f - q);
	}

	payload.bsdf = bsdfSample;

	
	/*
	if(any(isnan(payload.bsdf.bsdfDir)))
	{
		debugPrintfEXT("nan in step 5");
	}
	*/
	

	// Update a new ray path bounce direction
	payload.ray.direction = bsdfSample.bsdfDir;
	payload.ray.origin = worldPos;
}