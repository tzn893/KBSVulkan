#ifndef RANDOM_GLSL
#define RANDOM_GLSL

uint LFSR_Rand_Gen(uint n)
{
  // <<, ^ and & require GL_EXT_gpu_shader4.
  n = (n << 13) ^ n; 
  return (n * (n*n*15731+789221) + 1376312589) & 0x7fffffff;
}

float floatConstruct( uint m ) 
{
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat(m);       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}


float rand_i1_1(int idx, int seed)
{
  uint is = uint(idx) + uint(seed);
  return floatConstruct(LFSR_Rand_Gen(is));
}

vec2 rand_i1_2(int idx, int seed)
{
  uint is = uint(idx) + uint(seed);
    return vec2(floatConstruct(LFSR_Rand_Gen(is)),
        floatConstruct(LFSR_Rand_Gen(is + 100000)));
}

vec3 rand_i1_3(int idx, int seed)
{
  uint is = uint(idx) + uint(seed);
  return vec3(floatConstruct(LFSR_Rand_Gen(is)),
        floatConstruct(LFSR_Rand_Gen(is + 100000)),
        floatConstruct(LFSR_Rand_Gen(is + 200000)) );

}

float rand_2_1(vec2 co, float seed)
{
    return fract(sin(dot(co.xy + seed,vec2(12.9898,78.233))) * 43758.5453);
}

vec2 rand_2_2(vec2 uv, float seed)
{
  float r = rand_2_1(uv, seed);
  float g = rand_2_1(uv + vec2(1, 0.0), seed);
  
  return vec2(r,g);
}

#endif