#extension GL_EXT_debug_printf : enable
#extension GL_EXT_ray_tracing : require
#include "surfelPT.glsl"

layout(location = 1) rayPayloadInEXT   bool  isShadowed;

// do nothing
void main()
{
    //prd.color = vec3(1, 0, 0);
    isShadowed = false;
}