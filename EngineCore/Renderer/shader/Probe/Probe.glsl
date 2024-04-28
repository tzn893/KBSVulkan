#ifndef PROBE_GLSL
#define PROBE_GLSL

#include "SH.glsl"

struct Probe
{
    SH      rgb[3];
    vec3    position;
    float   sampleWeight;
};

struct Probe4
{
    SH4 rgb[3];
    vec3 position;
    float sampleWeight;
};

Probe interpolateProbe(Probe p1, Probe p2, float weight);
{
    Probe p;
    p.rgb[0] = interpolateSH(p1.rgb[0], p2.rgb[0], weight);
    p.rgb[1] = interpolateSH(p1.rgb[1], p2.rgb[1], weight);
    p.rgb[2] = interpolateSH(p1.rgb[2], p2.rgb[2], weight);

    return p;
}

Probe4 interpolateProbe(Probe4 p1, Probe4 p2, float weight);
{
    Probe4 p;
    p.rgb[0] = interpolateSH(p1.rgb[0], p2.rgb[0], weight);
    p.rgb[1] = interpolateSH(p1.rgb[1], p2.rgb[1], weight);
    p.rgb[2] = interpolateSH(p1.rgb[2], p2.rgb[2], weight);

    return p;
}

vec3 evalProbeRadiance(Probe probe, vec3 N)
{
    float r = EvalSH(probe.rgb[0], N);
    float g = EvalSH(probe.rgb[1], N);
    float b = EvalSH(probe.rgb[2], N);

    return vec3(r, g, b);
}

vec3 evalProbeRadiance(Probe4 probe, vec3 N)
{
    float r = EvalSH4(probe.rgb[0], N);
    float g = EvalSH4(probe.rgb[1], N);
    float b = EvalSH4(probe.rgb[2], N);

    return vec3(r, g, b);
}


#endif