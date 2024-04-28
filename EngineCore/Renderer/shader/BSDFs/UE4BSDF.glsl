/*
 *	Based on    https://github.com/knightcrawler25/GLSL-PathTracer
 *  UE4 SIGGAPH https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
 */


#ifndef UE4BSDF_GLSL
#define UE4BSDF_GLSL 

#ifndef KBS_PI
#define KBS_PI 3.1415926
#endif

#ifndef INV_KBS_PI
#define INV_KBS_PI 1 / KBS_PI
#endif

float SchlickFresnel(float u)
{
	float m = clamp(1.0 - u, 0.0, 1.0);
	return m * m * m * m * m; // power of 5
}

float DielectricFresnel(float cos_theta_i, float eta)
{
	float sinThetaTSq = eta * eta * (1.0f - cos_theta_i * cos_theta_i);

	// Total internal reflection
	if (sinThetaTSq > 1.0)
		return 1.0;

	float cos_theta_t = sqrt(max(1.0 - sinThetaTSq, 0.0));

	float rs = (eta * cos_theta_t - cos_theta_i) / (eta * cos_theta_t + cos_theta_i);
	float rp = (eta * cos_theta_i - cos_theta_t) / (eta * cos_theta_i + cos_theta_t);

	return 0.5f * (rs * rs + rp * rp);
}

/*
 * Generalized-Trowbridge-Reitz (D)
 */
float GTR1(float NDotH, float a)
{
	if (a >= 1.0)
		return (1.0 / KBS_PI);

	float a2 = a * a;
	float t = 1.0 + (a2 - 1.0) * NDotH * NDotH;

	return (a2 - 1.0) / (KBS_PI * log(a2) * t);
}

 /*
  * Generalized-Trowbridge-Reitz (D)
  * Describes differential area of microfacets for the surface normal
  */
float GTR2(float NDotH, float a)
{
	float a2 = a * a;
	float t = 1.0 + (a2 - 1.0) * NDotH * NDotH;
	return a2 / (KBS_PI * t * t);
}

 /*
  * The masking shadowing function Smith for GGX noraml distribution (G)
  */
float SmithGGX(float NDotv, float alphaG)	
{
	float a = alphaG * alphaG;
	float b = NDotv * NDotv;
	return 1.0 / (NDotv + sqrt(a + b - a * b));
}



vec3 UE4Eval(in vec3 albedo, in float metallic, in float roughness, in vec3 L, in vec3 N, in vec3 V)
{
	float NDotL = dot(N, L);
	float NDotV = dot(N, V);

	if (NDotL <= 0.0 || NDotV <= 0.0)
		return vec3(0.0);

	vec3 H = normalize(L + V);
	float NDotH = dot(N, H);
	float LDotH = dot(L, H);

	// Specular
	float specular = 0.5;
	vec3 specularCol = mix(vec3(1.0) * 0.08 * specular, albedo, metallic);
	float a = max(0.001, roughness);
	float D = GTR2(NDotH, a);
	float FH = SchlickFresnel(LDotH);
	vec3 F = mix(specularCol, vec3(1.0), FH);
	float roughg = (roughness * 0.5 + 0.5);
	roughg = roughg * roughg;

	float G = SmithGGX(NDotL, roughg) * SmithGGX(NDotV, roughg);

	return (albedo * INV_KBS_PI) * (1.0 - metallic) + F * D * G;
}

#endif