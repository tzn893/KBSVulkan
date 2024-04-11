#extension GL_EXT_debug_printf : enable
#extension GL_EXT_ray_tracing : require

#include "surfelPT.glsl"

layout(location = 2) rayPayloadInEXT hitPayload prd;

layout(set = 0, binding = 1) uniform LightBuffer
{
  vec3 lightVec;
  int  lightType;
  vec3 lightIntensity;
} lightBuffer;

void main()
{
    if(lightBuffer.lightType == 0)
    {
      prd.radiance = lightBuffer.lightIntensity * prd.attenuation;
    }
}