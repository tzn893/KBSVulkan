#pragma once
#include "Common.h"
#include "RenderResource.h"
#include "gvk.h"
#include "Scene/UUID.h"

namespace kbs
{
	using ShaderID = UUID;

	struct TextureCopyInfo
	{
		void* data;
		uint64_t dataSize;
		std::vector<VkBufferImageCopy> copyRegions;
		bool	generateMipmap;
	};


	class ComputeLikeKernel
	{
	public:
		ComputeLikeKernel(VkDevice device, std::vector<ptr<gvk::DescriptorSet>> descSet, ptr<gvk::Pipeline> pipeline, ShaderID computeShaderID);

		virtual void Dispatch(uint32_t x, uint32_t y, uint32_t z, VkCommandBuffer cmd) = 0;

		void UpdateBuffer(const char* name, VkBuffer buffer, uint32_t size, uint32_t offset);
		void UpdateBuffer( const char* name, ptr<RenderBuffer> buffer);
		void UpdateImageView( const char* name, VkImageView image, VkImageLayout layout,
			opt<uint32_t> arrayIdx,opt<VkSampler> imageSampler);
		void UpdateTexture( const char* name, ptr<Texture> texture, VkImageLayout layout)
		{
			UpdateImageView(name, texture->GetMainView(), layout, std::nullopt, texture->GetSampler());
		}

	protected:
		opt<uint64_t> FindSetIndex(const char* name, SpvReflectDescriptorType& descType, uint32_t& binding);

		std::vector<ptr<gvk::DescriptorSet>> m_DescriptorSets;
		ptr<gvk::Pipeline> m_ComputePipeline;
		VkDevice m_Device;
		ShaderID m_Shader;
	};

	class ComputeKernel : public ComputeLikeKernel
	{
	public:
		ComputeKernel(VkDevice device, std::vector<ptr<gvk::DescriptorSet>> descSet, ptr<gvk::Pipeline> pipeline, ShaderID computeShaderID)
			: ComputeLikeKernel(device, descSet, pipeline, computeShaderID) {}

		virtual void Dispatch(uint32_t x, uint32_t y, uint32_t z, VkCommandBuffer cmd) override;
	};

	class RayTracingKernel : public ComputeLikeKernel
	{
	public:
		RayTracingKernel(VkDevice device, std::vector<ptr<gvk::DescriptorSet>> descSet, ptr<gvk::Pipeline> pipeline, ShaderID computeShaderID);

		void UpdateAccelerationStructure(const char* name, ptr<gvk::TopAccelerationStructure> as);

		virtual void Dispatch(uint32_t x, uint32_t y, uint32_t z, VkCommandBuffer cmd)	override;
	private:
		
		ptr<gvk::RaytracingPipeline> m_RayTracingPipeline;
	};
	class RenderAPI
	{
	public:
		RenderAPI(const RenderAPI&) = default;
		RenderAPI(ptr<gvk::Context> ctx, ptr<gvk::DescriptorAllocator> descAllocator);
		RenderAPI() = default;

		ptr<gvk::DescriptorSet>		AllocateDescriptorSet(ptr<gvk::DescriptorSetLayout> layout);
		ptr<RenderBuffer>			CreateBuffer(VkBufferUsageFlags usage, uint32_t size, GVK_HOST_WRITE_PROPERTY prop);
		ptr<Texture>				CreateTexture(GvkImageCreateInfo imageInfo, GvkSamplerCreateInfo samplerInfo = GvkSamplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR));
		opt<ptr<gvk::Image>>		CreateImage(GvkImageCreateInfo imageInfo);
		opt<VkSampler>				CreateSampler(GvkSamplerCreateInfo info);
		opt<VkSampler>				CreateSamplerByName(const std::string& name);
		VkImageView					CreateImageMainView(ptr<gvk::Image> image);

		void						UploadBuffer(ptr<RenderBuffer> buffer, void* data, uint32_t size);
		void						UploadImage(ptr<gvk::Image> image, TextureCopyInfo textureInfo);

		opt<ptr<ComputeKernel>>		CreateComputeKernel(ShaderID shaderID);
		opt<ptr<RayTracingKernel>>	CreateRTKernel(ShaderID shaderID);

		void CopyRenderBuffer(ptr<RenderBuffer> src, ptr<RenderBuffer> target);

		opt<ptr<gvk::BottomAccelerationStructure>> CreateBottomAccelerationStructure(gvk::GvkBottomAccelerationStructureGeometryTriangles* bottomAS, uint32_t bottomAsCount);
		opt<ptr<gvk::TopAccelerationStructure>> CreateTopAccelerationStructure(gvk::GvkTopAccelerationStructureInstance* topAs, uint32_t topAsCount);

	private:
		ptr<gvk::Context> m_Ctx;
		ptr<gvk::DescriptorAllocator> m_DescAlloc;
	};
}