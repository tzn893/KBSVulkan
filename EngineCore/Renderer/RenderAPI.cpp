#include "RenderAPI.h"
#include "Core/Hasher.h"
#include "Core/Singleton.h"
#include "Renderer/Shader.h"
#include "Asset/AssetManager.h"

namespace kbs 
{
    class GlobalSamplerPool : public Is_Singleton
    {
    public:
        GlobalSamplerPool();

        VkSampler CreateSampler(ptr<gvk::Context> ctx, GvkSamplerCreateInfo& info);
        opt<VkSampler> CreateSamplerByName(ptr<gvk::Context> ctx, const std::string& name = "default");

        ~GlobalSamplerPool()
        {
            for (auto sampler : m_SamplerPool)
            {
                m_Ctx->DestroySampler(sampler.second);
            }
        }

    private:
        std::unordered_map<std::string, GvkSamplerCreateInfo> m_SamplerNameTable;
        std::unordered_map<uint64_t, VkSampler> m_SamplerPool;
        ptr<gvk::Context> m_Ctx;
    };

    RenderAPI::RenderAPI(ptr<gvk::Context> ctx, ptr<gvk::DescriptorAllocator> descAllocator)
    {
        m_Ctx = ctx;
        m_DescAlloc = descAllocator;
    }

    ptr<gvk::DescriptorSet> RenderAPI::AllocateDescriptorSet(ptr<gvk::DescriptorSetLayout> layout)
    {
        return m_DescAlloc->Allocate(layout).value();
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

    opt<VkSampler> RenderAPI::CreateSamplerByName(const std::string& name)
    {
        return Singleton::GetInstance<GlobalSamplerPool>()->CreateSamplerByName(m_Ctx, name);
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
        KBS_ASSERT(kbs_cover_flags(info.usage, VK_IMAGE_USAGE_TRANSFER_DST_BIT), "image must be copiable");

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

	kbs::opt<ptr<kbs::ComputeKernel>> RenderAPI::CreateComputeKernel(ShaderID shaderID)
	{
        ptr<ShaderManager> shaderManager = Singleton::GetInstance<AssetManager>()->GetShaderManager();
        ptr<ComputeShader> computeShader;

        {
            auto shader = shaderManager->Get(shaderID);
            if (!shader.has_value() || !shader.value()->IsComputeShader())
            {
                return std::nullopt;
            }
            computeShader = std::dynamic_pointer_cast<ComputeShader>(shader.value());
        }

        GvkComputePipelineCreateInfo computeCI;
        computeCI.shader = computeShader->GetGvkShader();

        auto sets = computeShader->GetGvkShader()->GetDescriptorSets().value();

        ptr<gvk::Pipeline> computePipeline;
        if(auto var = m_Ctx->CreateComputePipeline(computeCI); var.has_value())
        {
			computePipeline = var.value();
        }
        std::vector<ptr<gvk::DescriptorSet>> computeKernelDescSets;
        
        for (auto set : sets)
        {
            auto layout = computePipeline->GetInternalLayout(set->set);
            KBS_ASSERT(layout.has_value(), "compute pipeline must have a valid descriptor set layout for each set");
            ptr<gvk::DescriptorSet> ckSet = m_DescAlloc->Allocate(layout.value()).value();
            computeKernelDescSets.push_back(ckSet);
        }
        return std::make_shared<ComputeKernel>(m_Ctx->GetDevice(), computeKernelDescSets, computePipeline, shaderID);
	}

	kbs::opt<kbs::ptr<kbs::RayTracingKernel>> RenderAPI::CreateRTKernel(ShaderID shaderID)
	{
		ptr<ShaderManager> shaderManager = Singleton::GetInstance<AssetManager>()->GetShaderManager();
		ptr<RayTracingShader> rtShader;

		{
			auto shader = shaderManager->Get(shaderID);
			if (!shader.has_value() || !shader.value()->IsRayTracingShader())
			{
				return std::nullopt;
			}
            rtShader = std::dynamic_pointer_cast<RayTracingShader>(shader.value());
		}

		gvk::RayTracingPieplineCreateInfo pipelineCI = rtShader->OnPipelineStateCreate();
		ptr<gvk::RaytracingPipeline> rtPipeline;
		if (auto var = m_Ctx->CreateRaytracingPipeline(pipelineCI); var.has_value())
		{
            rtPipeline = var.value();
		}

		std::vector<ptr<gvk::DescriptorSet>> rtDescSets;
        std::vector<uint32_t> sets = rtShader->GetShaderReflection().GetOccupiedSetIndices();

		for (auto set : sets)
		{
			auto layout = rtPipeline->GetInternalLayout(set);
			KBS_ASSERT(layout.has_value(), "ray tracing pipeline must have a valid descriptor set layout for each set");
			ptr<gvk::DescriptorSet> ckSet = m_DescAlloc->Allocate(layout.value()).value();
			rtDescSets.push_back(ckSet);
		}

		return std::make_shared<RayTracingKernel>(m_Ctx->GetDevice(), rtDescSets, rtPipeline, shaderID);
	}

	void RenderAPI::CopyRenderBuffer(ptr<RenderBuffer> src, ptr<RenderBuffer> target)
    {
        KBS_ASSERT(src->GetBuffer()->GetSize() == target->GetBuffer()->GetSize(), "only buffer with the same size can be copied");

        VkBufferCopy copyRegion{};
        copyRegion.dstOffset = 0;
        copyRegion.srcOffset = 0;
        copyRegion.size = src->GetBuffer()->GetSize();

        auto queue = m_Ctx->PresentQueue();

        queue->SubmitTemporalCommand(
            [&](VkCommandBuffer cmd)
            {
                vkCmdCopyBuffer(cmd, src->GetBuffer()->GetBuffer(), target->GetBuffer()->GetBuffer(), 1, &copyRegion);
            }, gvk::SemaphoreInfo::None(), NULL, true
        );
    }

    opt<ptr<gvk::BottomAccelerationStructure>> RenderAPI::CreateBottomAccelerationStructure(gvk::GvkBottomAccelerationStructureGeometryTriangles* bottomAS, uint32_t bottomAsCount)
    {
        return m_Ctx->CreateBottomAccelerationStructure(gvk::View(bottomAS, 0, bottomAsCount)).value();
    }

    opt<ptr<gvk::TopAccelerationStructure>> RenderAPI::CreateTopAccelerationStructure(gvk::GvkTopAccelerationStructureInstance* topAs, uint32_t topAsCount)
    {
        return m_Ctx->CreateTopAccelerationStructure(gvk::View(topAs, 0, topAsCount)).value();
    }

    GlobalSamplerPool::GlobalSamplerPool()
    {
        {
            GvkSamplerCreateInfo sInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
            m_SamplerNameTable["default"] = sInfo;
            m_SamplerNameTable["linear_linear_linear"] = sInfo;
        }


        std::string nameTable[2] = { "nearest","linear" };
        for (int magF = 0;magF <= 1; magF++)
        {
            for (int minF = 0;minF <= 1;minF++)
            {
                for (int mipF = 0; mipF <= 1; mipF++)
                {
                    GvkSamplerCreateInfo sInfo( (VkFilter)magF, (VkFilter)minF, (VkSamplerMipmapMode)mipF);
                    std::string name = nameTable[magF] + "_" + nameTable[minF] + "_" + nameTable[mipF];
                    m_SamplerNameTable[name] = sInfo;
                }
            }
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

    opt<VkSampler> GlobalSamplerPool::CreateSamplerByName(ptr<gvk::Context> ctx, const std::string& name)
    {
        if (m_SamplerNameTable.count(name))
        {
            return CreateSampler(ctx, m_SamplerNameTable[name]);
        }
        return std::nullopt;
    }

	ComputeLikeKernel::ComputeLikeKernel(VkDevice device,std::vector<ptr<gvk::DescriptorSet>> descSet, ptr<gvk::Pipeline> pipeline,
        ShaderID shader)
        :m_ComputePipeline(pipeline),m_DescriptorSets(descSet), m_Device(device),m_Shader(shader)
    {

	}

	void ComputeKernel::Dispatch(uint32_t x, uint32_t y, uint32_t z, VkCommandBuffer cmd)
	{
        GvkBindPipeline(cmd, m_ComputePipeline);
        GvkDescriptorSetBindingUpdate descUpdate(cmd, m_ComputePipeline);
        for (auto desc : m_DescriptorSets)
        {
            descUpdate.BindDescriptorSet(desc);
        }
        descUpdate.Update();
        vkCmdDispatch(cmd, x, y, z);
	}

	void ComputeLikeKernel::UpdateBuffer(const char* name, ptr<RenderBuffer> buffer)
	{
		uint32_t targetSet, targetBinding;
        SpvReflectDescriptorType descType;
		if (auto set = FindSetIndex(name, descType, targetBinding); set.has_value())
		{
			targetSet = set.value();
		}
		else
		{
			return;
		}
        
        for (auto desc : m_DescriptorSets)
        {
            if (desc->GetSetIndex() == targetSet)
            {
                GvkDescriptorSetWrite().BufferWrite(desc,(VkDescriptorType)descType, targetBinding, buffer->GetBuffer()->GetBuffer(), 0,
                    buffer->GetBuffer()->GetSize()).Emit(m_Device);
                break;
            }
        }
	}

	void ComputeLikeKernel::UpdateBuffer( const char* name, VkBuffer buffer, uint32_t size, uint32_t offset)
	{
		uint32_t targetSet, targetBinding;
		SpvReflectDescriptorType descType;
		if (auto set = FindSetIndex(name, descType, targetBinding); set.has_value())
		{
			targetSet = set.value();
		}
		else
		{
			return;
		}

		for (auto desc : m_DescriptorSets)
		{
			if (desc->GetSetIndex() == targetSet)
			{
                GvkDescriptorSetWrite().BufferWrite(desc, (VkDescriptorType)descType, targetBinding, buffer, 0, size)
					.Emit(m_Device);
				break;
			}
		}
	}

	void ComputeLikeKernel::UpdateImageView( const char* name, VkImageView image, VkImageLayout layout,
		opt<uint32_t> arrayIdx, opt<VkSampler> imageSampler)
	{
		uint32_t targetSet, targetBinding;
		SpvReflectDescriptorType descType;
        if (auto set = FindSetIndex(name,descType, targetBinding); set.has_value())
        {
            targetSet = set.value();
        }
        else
        {
            return;
        }

        VkSampler sampler = imageSampler.has_value() ? imageSampler.value() : NULL;
        uint32_t aidx = arrayIdx.has_value() ? arrayIdx.value() : 0;

        for (auto desc: m_DescriptorSets)
        {
            if (desc->GetSetIndex() == targetSet)
            {
                GvkDescriptorSetWrite().ImageWrite(desc, (VkDescriptorType)descType, targetBinding, sampler, image, layout, aidx)
                    .Emit(m_Device);
                break;
            }
        }
	}

	kbs::opt<uint64_t> ComputeLikeKernel::FindSetIndex(const char* name, SpvReflectDescriptorType& descType, uint32_t& binding)
	{
        ptr<ShaderManager> shaderManager = Singleton::GetInstance<AssetManager>()->GetShaderManager();
        auto shader = shaderManager->Get(m_Shader).value();

        ShaderReflection reflect = shader->GetShaderReflection();
        if (auto buf = reflect.GetBuffer(name); buf.has_value())
        {
            binding = buf->binding;
            switch (buf->type)
            {
            case ShaderReflection::BufferType::Storage:
                descType = SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
            case ShaderReflection::BufferType::Uniform:
                descType = SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
            default:
                return std::nullopt;
            }

            return buf->set;
        }
        if (auto img = reflect.GetTexture(name); img.has_value())
        {
            binding = img->binding;
            switch (img->type)
            {
            case ShaderReflection::TextureType::CombinedImageSampler:
                descType = SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
            case ShaderReflection::TextureType::StorageImage:
                descType = SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE; break;
            default:
                return std::nullopt;
            }
            return img->set;
        }
        if (auto as = reflect.GetAS(name); as.has_value())
        {
            binding = as->binding;
            descType = SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            return as->set;
        }

        return std::nullopt;
	}

	RayTracingKernel::RayTracingKernel(VkDevice device, std::vector<ptr<gvk::DescriptorSet>> descSet, ptr<gvk::Pipeline> pipeline, ShaderID rtShaderID)
        : ComputeLikeKernel(device, descSet, pipeline, rtShaderID)
    {
        m_RayTracingPipeline = std::dynamic_pointer_cast<gvk::RaytracingPipeline>(pipeline);
	}

	void RayTracingKernel::UpdateAccelerationStructure(const char* name, ptr<gvk::TopAccelerationStructure> as)
	{
        uint64_t setIdx;
        SpvReflectDescriptorType descType;
        uint32_t binding;

        if (auto var = FindSetIndex(name, descType, binding); var.has_value())
        {
            setIdx = var.value();
        }
        else
        {
            return;
        }

        KBS_ASSERT(descType == SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, "{} (binding={},set={}) is not a acceleration structure", name, binding, setIdx);
        
        for (auto desc : m_DescriptorSets)
        {
            if (desc->GetSetIndex() == setIdx)
            {
				GvkDescriptorSetWrite()
					.AccelerationStructureWrite(desc, binding, as->GetAS())
				.Emit(m_Device);
            }
        }
	}

	void RayTracingKernel::Dispatch(uint32_t x, uint32_t y, uint32_t z, VkCommandBuffer cmd)
	{
		GvkDescriptorSetBindingUpdate descUpdate(cmd, m_ComputePipeline);
		for (auto desc : m_DescriptorSets)
		{
			descUpdate.BindDescriptorSet(desc);
		}
		descUpdate.Update();

        m_RayTracingPipeline->TraceRay(cmd, x, y, z);
	}

}

