#pragma once
#include "Common.h"


namespace kbs
{
	enum RenderPassFlagBit
	{
		RenderPass_Opaque = 1,
		RenderPass_Transparent = 2,
		RenderPass_Shadow = 4
	};
	using RenderPassFlags = uint64_t;

	enum RenderOptionFlagBit
	{
		RenderOption_DontCullByDistance = 1
	};
	using RenderOptionFlags = uint64_t;
}
