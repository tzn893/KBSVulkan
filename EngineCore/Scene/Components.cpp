#include "Components.h"

void kbs::RenderableComponent::AddRenderablePass(UUID targetMaterial, uint64_t renderOptionFlag)
{
	KBS_ASSERT(passCount < maxPassCount, "only {} passes is allowed for one renderable object", maxPassCount);

	RemoveRenderablePass(targetMaterial);

	targetMaterials[passCount] = targetMaterial;
	renderOptionFlags[passCount] = renderOptionFlag;
	
	passCount++;
}

void kbs::RenderableComponent::RemoveRenderablePass(UUID targetMaterial)
{
	uint32_t removeTargetIdx = maxPassCount;
	
	for (uint32_t i = 0;i < passCount; i++)
	{
		if (targetMaterials[i] == targetMaterial)
		{
			removeTargetIdx = i;
			break;
		}
	}

	if (removeTargetIdx < maxPassCount)
	{
		// don't change the order 
		for (uint32_t i = removeTargetIdx; (i + 1) < passCount; i++)
		{
			targetMaterials[i] = targetMaterials[i + 1];
		}
		passCount--;

	}
	
}
