#ifndef SH_GLSL
#define SH_GLSL

const float c1 = 0.429043, c2 = 0.511664, c3 = 0.743125, c4 = 0.886227, c5 = 0.247708;

//An Efficient Representation for Irradiance Environment Maps
float SH00(const vec3 d) {
  return 0.282095;
}

float SH1n1(const vec3 d) {
  return 0.488603 * d.y;
}

float SH10(const vec3 d) {
  return 0.488603 * d.z;
}

float SH1p1(const vec3 d) {
  return 0.488603 * d.x;
}

float SH2n2(const vec3 d) {
  return 1.092548 * d.x * d.y;
}

float SH2n1(const vec3 d) {
  return 1.092548 * d.y * d.z;
}

float SH20(const vec3 d) {
  return 0.315392 * (3.0 * d.z * d.z - 1);
}

float SH2p1(const vec3 d) {
  return 1.092548 * d.x * d.z;
}

float SH2p2(const vec3 d) {
  return 0.546274 * (d.x * d.x - d.y * d.y);
}

struct SH
{
    // (L_00, L_1-1, L_10, L_11)
    vec4    base0123;
    // (L_2-2, L_2-1, L_20, L_21)
    vec4    base4567;
    // (L_22)
    vec4    base8;
};

SH PackSH(float x, vec3 d)
{
    SH sh;
    sh.base0123 = vec4(SH00(d), SH1n1(d), SH10(d), SH1p1(d)) * x;
    sh.base4567 = vec4(SH2n2(d), SH2n1(d), SH20(d), SH2p1(d)) * x;
    sh.base8 = SH2p2(d) * x;

    return sh;
}

// implementation of paper https://cseweb.ucsd.edu/~ravir/papers/envmap/envmap.pdf
float EvalSH(SH sh, vec3 N)
{
    float x = N.x, y = N.y, z = N.z;
    float x2 = x * x, y2 = y * y, z2 = z * z;
    float xy = x * y, xz = x * z, yz = y * z;

    float L00 = sh.base0123.x;
    
    float L1_1  = sh.base0123.y;
    float L10   = sh.base0123.z;
    float L11   = sh.base0123.w;

    float L2_2  = sh.base4567.x;
    float L2_1  = sh.base4567.y;
    float L20   = sh.base4567.z;
    float L21   = sh.base4567.w;
    float L22   = sh.base8;

    return L22 * c1 * (x2 - y2) + c3 * L20 * z2 + c4 * L00 - c5 * L20 + 
    2 * c1 * (L2_2 * xy + L21 * xz + L2_1 * yz) +
    2 * c2 * (L11 * x + L1_1 * y + L_10 * z);
}

SH interpolateSH(SH sh1, SH sh2, float weight)
{
    SH sh;
    sh.base0123 = mix(sh1.base0123, sh2.base0123, weight);
    sh.base4567 = mix(sh1.base4567, sh2.base4567, weight);
    sh.base8 = mix(sh1.base8, sh2.base8, weight);

    return sh;
}

struct SH4
{
    vec4 base0123;
};

SH4 PackSH4(float x, vec3 d)
{
    SH4 sh;
    sh.base0123 = vec4(SH00(d), SH1n1(d), SH10(d), SH1p1(d)) * x;

    return sh;
}

SH4 interpolateSH(SH4 sh1, SH4 sh2, float weight)
{
    SH4 sh;
    sh.base0123 = mix(sh1.base0123, sh2.base0123, weight);
    return sh;
}

float EvalSH(SH4 sh, vec3 N)
{
    float x = N.x, y = N.y, z = N.z;

    float L00 = sh.base0123.x;
    
    float L1_1  = sh.base0123.y;
    float L10   = sh.base0123.z;
    float L11   = sh.base0123.w;

    return c4 * L00 + 2 * c2 * (L11 * x + L1_1 * y + L_10 * z);
}

#endif