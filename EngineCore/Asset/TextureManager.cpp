#include "TextureManager.h"
#include <filesystem>
#include "Renderer/RenderAPI.h"
namespace fs = std::filesystem;

namespace kbs
{
    TextureID kbs::ManagedTexture::GetTextureID()
    {
        return m_TextureID;
    }

    std::string kbs::ManagedTexture::GetPath()
    {
        return m_Path;
    }


	void TextureManager::LoadDefaultTextures(RenderAPI& api)
	{
        GvkImageCreateInfo info = GvkImageCreateInfo::Image2D(VK_FORMAT_R8G8B8A8_UNORM, 1, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT);
        
		ptr<gvk::Image> normal = api.CreateImage(info).value();
		ptr<gvk::Image> white = api.CreateImage(info).value();
		ptr<gvk::Image> black = api.CreateImage(info).value();

        char normalData[] = {0, 0, 255, 255};
        char whiteData[] = { 255,255,255,255 };
        char blackData[] = { 0, 0, 0, 255 };

        TextureCopyInfo cpyInfo;
        cpyInfo.generateMipmap = false;
        cpyInfo.data = normalData;
        cpyInfo.dataSize = sizeof(normalData);
       

        VkBufferImageCopy biCpyRegion;
        biCpyRegion.bufferImageHeight = 1;
        biCpyRegion.bufferRowLength = 1;
        biCpyRegion.bufferOffset = 0;
        biCpyRegion.imageOffset = {0, 0, 0};
        biCpyRegion.imageExtent = { 1, 1, 1 };
        
        biCpyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        biCpyRegion.imageSubresource.baseArrayLayer = 0;
        biCpyRegion.imageSubresource.layerCount = 1;
        biCpyRegion.imageSubresource.mipLevel = 0;
        
        cpyInfo.copyRegions.push_back(biCpyRegion);
        api.UploadImage(normal, cpyInfo);
        VkImageView normalView = normal->CreateView(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1, VK_IMAGE_VIEW_TYPE_2D).value();
        VkSampler normalSampler = api.CreateSampler(GvkSamplerCreateInfo()).value();
        this->m_DefaultTextureNormal = Attach(normal, normalSampler, normalView, "__internal_textures/normal").value();


        cpyInfo.data = whiteData;
        api.UploadImage(white, cpyInfo);
        VkImageView whiteView = white->CreateView(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1, VK_IMAGE_VIEW_TYPE_2D).value();
        VkSampler whiteSampler = api.CreateSampler(GvkSamplerCreateInfo()).value();
        this->m_DefaultTextureWhite = Attach(white, whiteSampler, whiteView, "__internal_textures/white").value();

        cpyInfo.data = blackData;
        api.UploadImage(black, cpyInfo);
        VkImageView blackView = black->CreateView(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1, VK_IMAGE_VIEW_TYPE_2D).value();
        VkSampler blackSampler = api.CreateSampler(GvkSamplerCreateInfo()).value();
        this->m_DefaultTextureBlack = Attach(black, blackSampler, blackView, "__internal_textures/black").value();
	}

	opt<TextureID> TextureManager::Attach(ptr<gvk::Image> image, VkSampler sampler, VkImageView view, const std::string& path)
    {
        std::string fullPath = fs::absolute(fs::path(path)).string();

        if (m_TextureByPath.count(fullPath))
        {
            return std::nullopt;
        }
        TextureID id = UUID::GenerateUncollidedID(m_Textures);
        m_Textures[id] = std::make_shared<ManagedTexture>(image, sampler, view, fullPath, id);
        m_TextureByPath[fullPath] = id;

        return id;
    }

    /*
    opt<TextureID> TextureManager::Load(const std::string& path, RenderAPI& api, GvkSamplerCreateInfo samplerInfo)
    {
        fs::path fsPath = path;
        fsPath = fs::absolute(fsPath);

        // TODO reload texture
        if (GetTextureByPath(fsPath.string()).has_value())
        {
            return std::nullopt;
        }
        
        std::string extension = fsPath.extension().string();

        GvkImageCreateInfo imageInfo{};
        auto image = api.CreateImage(imageInfo);
        auto sampler = api.CreateSampler(samplerInfo);
        
        VkImageViewType viewType;
        if (imageInfo.extent.depth > 1)
        {
            viewType = VK_IMAGE_VIEW_TYPE_3D;
        }
        else if (imageInfo.extent.height > 1)
        {
            if (imageInfo.arrayLayers > 1)
            {
                viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            }
            else
            {
                viewType = VK_IMAGE_VIEW_TYPE_2D;
            }
        }
        else
        {
            if (imageInfo.arrayLayers > 1)
            {
                viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            }
            else
            {
                viewType = VK_IMAGE_VIEW_TYPE_1D;
            }
        }
        auto view = image.value()->CreateView(gvk::GetAllAspects(imageInfo.format), 0, imageInfo.mipLevels, 0, imageInfo.arrayLayers, viewType);
        // TODO load texture

        TextureID id = UUID::GenerateUncollidedID(m_Textures);
        m_Textures[id] = std::make_shared<ManagedTexture>(image.value(), sampler.value(), view.value(), path, id);
        m_TextureByPath[fsPath.string()] = id;
        return id;
    }
    */
    opt<ptr<ManagedTexture>> TextureManager::GetTextureByID(TextureID id)
    {
        if (!m_Textures.count(id))
        {
            return std::nullopt;
        }
        return m_Textures[id];
    }

    opt<ptr<ManagedTexture>> TextureManager::GetTextureByPath(const std::string& path)
    {
        std::string fullPath = fs::absolute(fs::path(path)).string();
        if (!m_TextureByPath.count(fullPath))
        {
            return std::nullopt;
        }
        return  m_Textures[m_TextureByPath[fullPath]];
    }

	kbs::TextureID TextureManager::GetDefaultWhite()
	{
        return m_DefaultTextureWhite;
	}

	kbs::TextureID TextureManager::GetDefaultNormal()
	{
        return m_DefaultTextureNormal;
	}

	kbs::TextureID TextureManager::GetDefaultBlack()
	{
        return m_DefaultTextureBlack;
	}

}


