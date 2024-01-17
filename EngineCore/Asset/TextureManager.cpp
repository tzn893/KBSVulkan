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


    opt<TextureID> TextureManager::Attach(ptr<gvk::Image> image, VkSampler sampler, VkImageView view, const std::string& path)
    {
        if (m_TextureByPath.count(path))
        {
            return std::nullopt;
        }
        TextureID id = UUID::GenerateUncollidedID(m_Textures);
        m_Textures[id] = std::make_shared<ManagedTexture>(image, sampler, view, path, id);
        m_TextureByPath[path] = id;

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

}


