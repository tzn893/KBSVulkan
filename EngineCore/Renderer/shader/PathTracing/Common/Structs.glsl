/*
 *  Global structure used in shaders
 */
#ifndef PT_STRUCTURE_GLSL
#define PT_STRUCTURE_GLSL

 #extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#define PI        3.14159265358979323846
#define TWO_PI    6.28318530717958647692
#define INV_PI    0.31830988618379067154
#define INV_2PI   0.15915494309189533577

#define EPS       0.001
#define INFINITY  1000000.0
#define MINIMUM   0.00001

// See Random.glsl for more details
uint seed = 0;

// Lights types
int AREA_LIGHT = 0;
int SPHERE_LIGHT = 1;
int DIRECTIONAL_LIGHT = 2;
int CONSTANT_LIGHT = 3;
int POINT_LIGHT = 4;

// Integrators types
int PATH_TRACER_DEFAULT = 0;
int PATH_TRACER_MSM = 1;
int AMBIENT_OCCLUSION = 2;

struct Material 
{ 
	vec4 albedo;
	vec4 emission;
	vec4 extinction;
	
	float metallic;
    float roughness;
    float subsurface;
    float specularTint;
    
    float sheen;
    float sheenTint;
    float clearcoat;
    float clearcoatGloss;
    
    float transmission;
    float ior;
    float atDistance;
	float __padding;
	
	int albedoTexID;
	int metallicRoughnessTexID;
	int normalmapTexID;
	int heightmapTexID;
};

struct Ray
{ 
	vec3 origin; 
	vec3 direction; 
};

struct Light
{ 
	vec3 position;
	float area;
	vec3 emission;
	int type;
	vec3 u;
	float radius;
	vec3 v;
	float __padding;
};

struct Uniform
{
	mat4 view;
	mat4 proj;
	vec3 cameraPos;
	uint lights;
	bool doubleSidedLight;
	uint spp;
	uint maxDepth;
	uint frame;
	float aperture;
	float focalDistance;
	//float hdrMultiplier;
	//float hdrResolution;
	float AORayLength;
	int integratorType;
};

struct ComputeUniform
{
	uint iteration;
	float colorPhi;
	float normalPhi;
	float positionPhi;
	float stepWidth;
};

struct LightSample
{ 
	vec3 normal; 
	vec3 position;
	vec3 emission; 
	float pdf;
	bool  isInfinite;
	bool  isDelta;
};

struct BsdfSample
{
	vec3 bsdfDir; 
	float pdf; 
};

struct RayPayload	
{
	Ray ray;
	BsdfSample bsdf;
	vec3 radiance;
	vec3 absorption;
	vec3 beta;
	vec3 worldPos;
	vec3 normal;
	vec3 ffnormal;
	uint depth;
	bool stop;
	float eta;
};

struct ObjDesc
{
    uint64_t vertexBufferAddr;
    uint64_t indiceBufferAddr;
    uint64_t materialSetIndex;
	int indexPrimitiveOffset;
	int vertexOffset;
};

#endif