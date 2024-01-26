#include "RenderAPI.h"
#include "Core/Hasher.h"
#include "Core/Singleton.h"

namespace kbs 
{
    class GlobalSamplerPool : public Is_Singleton
    {
    public:
        GlobalSamplerPool() = default;
        
        VkSampler CreateSampler(ptr<gvk::Context> ctx, GvkSamplerCreateInfo& info);

        ~GlobalSamplerPool()
        {
            for (auto sampler : m_SamplerPool)
            {
                m_Ctx->DestroySampler(sampler.second);
            }
        }

    private:
        std::unordered_map<uint64_t, VkSampler> m_SamplerPool;
        ptr<gvk::Context> m_Ctx;
    };
    

    RenderAPI::RenderAPI(ptr<gvk::Context> ctx)
    {
        m_Ctx = ctx;
    }

    ptr<RenderBuffer> kbs::RenderAPI::CreateBuffer(VkBufferUsageFlags usage, uint32_t size, GVK_HOST_WRITE_PROPERTY prop)
    {
        ptr<gvk::Buffer> buffer = m_Ctx->CreateBuffer(usage, size, prop).value();

        return std::make_shared<RenderBuffer>(buffer);
    }


    ptr<Texture> RenderAPI::CreateTexture(GvkImageCreateInfo imageInfo, GvkSamplerCreateInfo samplerInfo)
    {
        KBS_ASSERT(samplerInfo.pNext == NULL && samplerInfo.sType == VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, "we don't support extended sampler");
        VkSampler sampler = Singleton::GetInstance<GlobalSamplerPool>()->CreateSampler(m_Ctx, samplerInfo);

        auto image = m_Ctx->CreateImage(imageInfo);
        auto view = CreateImageMainView(image.value());

        return std::make_shared<Texture>(image.value(), sampler, view);
    }

    opt<ptr<gvk::Image>> RenderAPI::CreateImage(GvkImageCreateInfo imageInfo)
    {
        return m_Ctx->CreateImage(imageInfo);
    }

    opt<VkSampler> RenderAPI::CreateSampler(GvkSamplerCreateInfo info)
    {
        return Singleton::GetInstance<GlobalSamplerPool>()->CreateSampler(m_Ctx, info);
    }

    VkImageView RenderAPI::CreateImageMainView(ptr<gvk::Image> image)
    {
        GvkImageCreateInfo imageInfo = image->Info();
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
        auto view = image->CreateView(gvk::GetAllAspects(imageInfo.format), 0, imageInfo.mipLevels, 0, imageInfo.arrayLayers, viewType);
        return view.value();
    }

    static void SetImageLayout(
        VkCommandBuffer cmdbuffer,
        VkImage image,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkImageSubresourceRange subresourceRange,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
    {
        // Create an image barrier object
        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = subresourceRange;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (oldImageLayout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            // Image layout is undefined (or does not matter)
            // Only valid as initial layout
            // No flags required, listed only for completeness
            imageMemoryBarrier.srcAccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // Image is preinitialized
            // Only valid as initial layout for linear images, preserves memory contents
            // Make sure host writes have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image is a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image is a depth/stencil attachment
            // Make sure any writes to the depth/stencil buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image is a transfer source
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image is a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image is read by a shader
            // Make sure any shader reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (newImageLayout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image will be used as a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image will be used as a transfer source
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image will be used as a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image layout will be used as a depth/stencil attachment
            // Make sure any writes to depth/stencil buffer have been finished
            imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image will be read in a shader (sampler, input attachment)
            // Make sure any writes to the image have been finished
            if (imageMemoryBarrier.srcAccessMask == 0)
            {
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
        }

        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(
            cmdbuffer,
            srcStageMask,
            dstStageMask,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);
    }


    void RenderAPI::UploadBuffer(ptr<RenderBuffer> buffer, void* data, uint32_t size)
    {
        KBS_ASSERT(buffer->GetBuffer()->GetSize() == size, "the data's size uploaded to buffer must match the size of buffer");
        
        ptr<gvk::Buffer> stagingBuffer = m_Ctx->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, GVK_HOST_WRITE_SEQUENTIAL).value();
        stagingBuffer->Write(data, 0, size);
        
        ptr<gvk::CommandQueue> queue = m_Ctx->PresentQueue();
        queue->SubmitTemporalCommand(
            [&](VkCommandBuffer cmd)
            {
                VkBufferCopy copy{};
                copy.srcOffset = 0;
                copy.size = size;
                copy.dstOffset = 0;
                vkCmdCopyBuffer(cmd, stagingBuffer->GetBuffer(), buffer->GetBuffer()->GetBuffer(), 1, &copy);
            }, 
            gvk::SemaphoreInfo::None(), NULL, true);
        stagingBuffer = nullptr;
    }

    void RenderAPI::UploadImage(ptr<gvk::Image> image, TextureCopyInfo textureInfo)
    {
        GvkImageCreateInfo info = image->Info();
        KBS_ASSERT((textureInfo.generateMipmap && textureInfo.copyRegions.size() == 1) || (!textureInfo.generateMipmap && info.mipLevels == textureInfo.copyRegions.size()), "only one region is allowed if mipmap is generated automatically");
        KBS_ASSERT(kbs_cover_flags(info.usage, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT), "image must be copiable");

        ptr<gvk::CommandQueue> queue = m_Ctx->PresentQueue();
        ptr<gvk::Buffer> stagingBuffer = m_Ctx->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, textureInfo.dataSize, GVK_HOST_WRITE_SEQUENTIAL).value();
        stagingBuffer->Write(textureInfo.data, 0, textureInfo.dataSize);


        if (textureInfo.generateMipmap)
        {
            auto imageCopyCmd = [&](VkCommandBuffer cmd)
            {
                VkImageSubresourceRange subresourceRange = {};
                subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                subresourceRange.levelCount = 1;
                subresourceRange.layerCount = 1;

                {
                    VkImageMemoryBarrier imageMemoryBarrier{};
                    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    imageMemoryBarrier.srcAccessMask = 0;
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageMemoryBarrier.image = image->GetImage();
                    imageMemoryBarrier.subresourceRange = subresourceRange;
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
                }

                vkCmdCopyBufferToImage(cmd, stagingBuffer->GetBuffer(), image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, textureInfo.copyRegions.size(), 
                    textureInfo.copyRegions.data());

                {
                    VkImageMemoryBarrier imageMemoryBarrier{};
                    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    imageMemoryBarrier.image = image->GetImage();
                    imageMemoryBarrier.subresourceRange = subresourceRange;
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
                }
            };

            auto blitImageCmd = [&](VkCommandBuffer cmd)
            {
                VkImageSubresourceRange subresourceRange = {};
                subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                subresourceRange.baseMipLevel = 0;
                subresourceRange.levelCount = info.mipLevels;
                subresourceRange.baseArrayLayer = 0;
                subresourceRange.layerCount = 1;

                for (uint32_t i = 1; i < info.mipLevels; i++) {
                    VkImageBlit imageBlit{};

                    imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageBlit.srcSubresource.layerCount = 1;
                    imageBlit.srcSubresource.mipLevel = i - 1;
                    imageBlit.srcOffsets[1].x = int32_t(info.extent.width >> (i - 1));
                    imageBlit.srcOffsets[1].y = int32_t(info.extent.height >> (i - 1));
                    imageBlit.srcOffsets[1].z = 1;

                    imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageBlit.dstSubresource.layerCount = 1;
                    imageBlit.dstSubresource.mipLevel = i;
                    imageBlit.dstOffsets[1].x = int32_t(info.extent.width >> i);
                    imageBlit.dstOffsets[1].y = int32_t(info.extent.height >> i);
                    imageBlit.dstOffsets[1].z = 1;

                    VkImageSubresourceRange mipSubRange = {};
                    mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    mipSubRange.baseMipLevel = i;
                    mipSubRange.levelCount = 1;
                    mipSubRange.layerCount = 1;

                    {
                        VkImageMemoryBarrier imageMemoryBarrier{};
                        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                        imageMemoryBarrier.srcAccessMask = 0;
                        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                        imageMemoryBarrier.image = image->GetImage();
                        imageMemoryBarrier.subresourceRange = mipSubRange;
                        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
                    }

                    vkCmdBlitImage(cmd, image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

                    {
                        VkImageMemoryBarrier imageMemoryBarrier{};
                        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                        imageMemoryBarrier.image = image->GetImage();
                        imageMemoryBarrier.subresourceRange = mipSubRange;
                        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
                    }
                }

                subresourceRange.levelCount = info.mipLevels;

                {
                    VkImageMemoryBarrier imageMemoryBarrier{};
                    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    imageMemoryBarrier.image = image->GetImage();
                    imageMemoryBarrier.subresourceRange = subresourceRange;
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
                }
            };

            queue->SubmitTemporalCommand(imageCopyCmd, gvk::SemaphoreInfo(), NULL, true);
            if (info.mipLevels > 1)
            {
                queue->SubmitTemporalCommand(blitImageCmd, gvk::SemaphoreInfo(), NULL, true);
            }
        }
        else
        {

            auto copyCmd = [&](VkCommandBuffer cmd)
            {
                VkImageSubresourceRange subresourceRange = {};
                subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                subresourceRange.baseMipLevel = 0;
                subresourceRange.levelCount = info.mipLevels;
                subresourceRange.layerCount = 1;

                SetImageLayout(cmd, image->GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
                vkCmdCopyBufferToImage(cmd, stagingBuffer->GetBuffer(), image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(textureInfo.copyRegions.size()), textureInfo.copyRegions.data());
                SetImageLayout(cmd, image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
            };

            queue->SubmitTemporalCommand(copyCmd, gvk::SemaphoreInfo(), NULL, true);
        }
    }

    VkSampler GlobalSamplerPool::CreateSampler(ptr<gvk::Context> ctx, GvkSamplerCreateInfo& info)
    {
        if (m_Ctx == nullptr)
        {
            m_Ctx = ctx;
        }
        else
        {
            KBS_ASSERT(m_Ctx == ctx, "only one vulkan context in one application");
        }

        uint64_t id = Hasher::HashMemoryContent(info);
        if (!m_SamplerPool.count(id))
        {
            auto sampler = m_Ctx->CreateSampler(info);
            m_SamplerPool[id] = sampler.value();
        }
        
        return m_SamplerPool[id];
    }

}

