#pragma once
#include "Common.h"
#include "RenderResource.h"
#include "gvk.h"


namespace kbs
{
	class RenderAPI
	{
	public:
		RenderAPI(const RenderAPI&) = default;
		RenderAPI(ptr<gvk::Context> ctx);
		RenderAPI() = default;

		ptr<RenderBuffer> CreateBuffer(VkBufferUsageFlags usage, uint32_t size, GVK_HOST_WRITE_PROPERTY prop);
		// ptr<Texture>	  CreateTexture();

	private:
		ptr<gvk::Context> m_Ctx;
	};
}