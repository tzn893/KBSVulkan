// HDR specific functions

#ifndef PT_HDR_GLSL
#define PT_HDR_GLSL

float envPdf()
{
	vec3 direction = gl_WorldRayDirectionEXT;
	float theta = acos(clamp(direction.y, -1.0, 1.0));
	vec2 uv = vec2((PI + atan(direction.z, direction.x)) * INV_2PI, theta * INV_PI);
	float pdf = texture(HDRs[1], uv).y * texture(HDRs[2], vec2(0., uv.y)).y;
	return (pdf * ubo.hdrResolution) / (TWO_PI * PI * sin(theta));
}

vec4 envSample(inout vec3 color)
{
	float r1 = rnd(seed);
	float r2 = rnd(seed);

	float v = texture(HDRs[2], vec2(0., r1)).x; // marginal
	float u = texture(HDRs[1], vec2(r2, v)).x;  // conditional

	color = texture(HDRs[0], vec2(u, v)).xyz * ubo.hdrMultiplier;
	float pdf = texture(HDRs[1], vec2(u, v)).y * texture(HDRs[2], vec2(0., v)).y;

	float phi = u * TWO_PI;
	float theta = v * PI;

	if (sin(theta) == 0.0)
		pdf = 0.0;

	return vec4(
		-sin(theta) * cos(phi), 
		cos(theta),
		-sin(theta)*sin(phi),
		(pdf * ubo.hdrResolution) / (TWO_PI * PI * sin(theta))
	);
}

#endif