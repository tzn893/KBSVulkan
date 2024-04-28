#pragma once
#include "Common.h"
#include "vkrg/pass.h"
#include "vkrg/graph.h"
#include "Scene/Scene.h"
#include "Renderer/RenderCamera.h"
#include "Renderer/RenderResource.h"
#include "Renderer/Renderer.h"
#include "Renderer/Shader.h"

namespace kbs
{

	class DeferredPass : public RendererPass
	{
	public:

		virtual void OnSceneRender(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd,
			RenderCamera& camera, ptr<Scene> scene) override;

		virtual void GetClearValue(uint32_t attachment, VkClearValue& value) override;

		virtual void GetAttachmentStoreLoadOperation(uint32_t attachment, VkAttachmentLoadOp& loadOp, VkAttachmentStoreOp& storeOp,
			VkAttachmentLoadOp& stencilLoadOp, VkAttachmentStoreOp& stencilStoreOp) override;

		virtual bool OnValidationCheck(std::string& msg) override;

		ptr<GraphicsShader>	 GetMRTShader();

	protected:
		virtual void Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc,
			ptr<gvk::Context> ctx) override;

		bool CheckAttachment(vkrg::RenderPassAttachment attachment, std::string attachment_name, std::string& msg);

		vkrg::RenderPassAttachment normal;
		vkrg::RenderPassAttachment color;
		vkrg::RenderPassAttachment material;
		vkrg::RenderPassAttachment depthStencil;

		ptr<GraphicsShader>		m_MRTShader;
	};

	class DeferredShadingPass : public ComputePass
	{
	public:
		
		void AttachScene(ptr<Scene> scene, opt<Entity> mainLight, opt<kbs::mat4> mainLightMVP);
		void ResizeScreen(uint32_t width, uint32_t height);

	protected:
		virtual void	Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc) override;
		virtual void	OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) override;

		ptr<RenderBuffer>	m_UniformBuffer;
		ptr<RenderBuffer>	m_LightBuffer;

		vkrg::RenderPassAttachment m_ColorImageAttachment;
		opt<vkrg::RenderPassAttachment> m_ShadowMapAttachment;
		vkrg::RenderPassAttachment m_GBufferDepthAttachment;
		vkrg::RenderPassAttachment m_GBufferColorAttachment;
		vkrg::RenderPassAttachment m_GBufferNormalAttachment;
		vkrg::RenderPassAttachment m_GBufferMaterialAttachment;

		ptr<ComputeKernel>	m_DeferredShadingKernel;

		VkSampler m_DefaultSampler;
		VkSampler m_NearestSampler;

		VkImageView m_WhiteImageView = NULL;

		uint32_t m_Width, m_Height;
	};
}

