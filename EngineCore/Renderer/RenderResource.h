#pragma once
#include "Common.h"
#include "gvk.h"


namespace kbs
{
	class RenderBuffer
	{
	public:
		RenderBuffer(ptr<gvk::Buffer> buffer);
		
		ptr<gvk::Buffer> GetBuffer();
	
	private:
		ptr<gvk::Buffer> buffer;
	};

	class Texture
	{
	public:
		Texture(ptr<gvk::Image> image, VkSampler sampler, VkImageView view);

		ptr<gvk::Image> GetImage();
		VkSampler		GetSampler();
		VkImageView		GetMainView();

	private:
		ptr<gvk::Image> m_Image;
		VkSampler		m_Sampler;
		VkImageView		m_MainView;
	};

}