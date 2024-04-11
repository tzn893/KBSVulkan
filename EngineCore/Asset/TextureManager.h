#pragma once
#include "Scene/UUID.h"
#include "Renderer/RenderResource.h"

namespace kbs
{
	using TextureID = UUID;
	class RenderAPI;

	class ManagedTexture : public Texture
	{
	public:
		ManagedTexture(ptr<gvk::Image> image, VkSampler sampler, VkImageView view, const std::string& path, TextureID id)
			:Texture(image, sampler, view),m_Path(path), m_TextureID(id) {}

		TextureID			GetTextureID();
		std::string			GetPath();

	private:
		TextureID m_TextureID;
		std::string m_Path;
	};

	class TextureManager
	{
	public:
		TextureManager() = default;

		void LoadDefaultTextures(RenderAPI& api);

		opt<TextureID> Attach(ptr<gvk::Image> image, VkSampler sampler, VkImageView view, const std::string& path);
		// opt<TextureID> Load(const std::string& path, RenderAPI& api, GvkSamplerCreateInfo samplerInfo = GvkSamplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR));
		opt<ptr<ManagedTexture>> GetTextureByID(TextureID id);
		opt<ptr<ManagedTexture>> GetTextureByPath(const std::string& path);

		TextureID	GetDefaultWhite();
		TextureID	GetDefaultNormal();
		TextureID	GetDefaultBlack();

	private:
		TextureID		m_DefaultTextureWhite;
		TextureID		m_DefaultTextureBlack;
		TextureID		m_DefaultTextureNormal;

		std::unordered_map<TextureID, ptr<ManagedTexture>> m_Textures;
		std::unordered_map<std::string, TextureID> m_TextureByPath;
	};

}