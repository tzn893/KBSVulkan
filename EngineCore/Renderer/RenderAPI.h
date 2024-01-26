#pragma once
#include "Common.h"
#include "RenderResource.h"
#include "gvk.h"


namespace kbs
{
	struct TextureCopyInfo
	{
		void* data;
		uint64_t dataSize;
		std::vector<VkBufferImageCopy> copyRegions;
		bool	generateMipmap;
	};


	class RenderAPI
	{
	public:
		RenderAPI(const RenderAPI&) = default;
		RenderAPI(ptr<gvk::Context> ctx);
		RenderAPI() = default;

		ptr<RenderBuffer>			CreateBuffer(VkBufferUsageFlags usage, uint32_t size, GVK_HOST_WRITE_PROPERTY prop);
		ptr<Texture>				CreateTexture(GvkImageCreateInfo imageInfo, GvkSamplerCreateInfo samplerInfo = GvkSamplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR));
		opt<ptr<gvk::Image>>		CreateImage(GvkImageCreateInfo imageInfo);
		opt<VkSampler>				CreateSampler(GvkSamplerCreateInfo info);
		VkImageView					CreateImageMainView(ptr<gvk::Image> image);


		void						UploadBuffer(ptr<RenderBuffer> buffer, void* data, uint32_t size);
		void						UploadImage(ptr<gvk::Image> image, TextureCopyInfo textureInfo);

	private:
		ptr<gvk::Context> m_Ctx;
	};
}