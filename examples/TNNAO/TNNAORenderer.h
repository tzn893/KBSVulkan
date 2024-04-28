#pragma once
#include "Renderer/Renderer.h"
#include "Renderer/DeferredPass.h"
#include "Renderer/ShadowPass.h"

namespace kbs
{

	class TNNAOPass : public ComputePass
	{
	public:
		void ResizeScreen(uint32_t width, uint32_t height);

		void SetUniformBuffer(ptr<RenderBuffer> buffer);

	protected:
		virtual void	Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc) override;
		virtual void	OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) override;

		ptr<RenderBuffer>	m_UniformBuffer;

		vkrg::RenderPassAttachment m_ColorImageAttachment;
		vkrg::RenderPassAttachment m_GBufferDepthAttachment;
		vkrg::RenderPassAttachment m_GBufferNormalAttachment;

		ptr<ComputeKernel>	m_TNNAOKernel;

		VkSampler m_DefaultSampler;
		VkSampler m_NearestSampler;

		uint32_t m_Width, m_Height;
	};


	class TNNAODeferredShadingPass : public ComputePass
	{
	public:

		void AttachScene(ptr<Scene> scene, opt<Entity> mainLight, opt<kbs::mat4> mainLightMVP);
		void ResizeScreen(uint32_t width, uint32_t height);

		ptr<RenderBuffer>	GetUniformBuffer();

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
		vkrg::RenderPassAttachment m_AOAttachment;

		ptr<ComputeKernel>	m_DeferredShadingKernel;

		VkSampler m_DefaultSampler;
		VkSampler m_NearestSampler;

		VkImageView m_WhiteImageView = NULL;

		uint32_t m_Width, m_Height;
	};

	class TNNAORenderer : public kbs::Renderer
	{
	public:
		TNNAORenderer() = default;

		ptr<DeferredPass> GetDeferredPass();

	protected:
		virtual vkrg::RenderPassHandle GetMaterialTargetRenderPass(RenderPassFlags flag) override;
		virtual bool InitRenderGraph(ptr<kbs::Window> window, ptr<vkrg::RenderGraph> renderGraph) override;

		virtual void OnSceneRender(ptr<Scene> scene) override;

		ptr<DeferredPass>				m_DeferredPass;
		vkrg::RenderPassHandle			m_DeferredPassHandle;
		ptr<TNNAODeferredShadingPass>	m_DeferredShadingPass;
		vkrg::RenderPassHandle			m_DeferredShadingPassHandle;
		ptr<ShadowPass>					m_ShadowPass;
		vkrg::RenderPassHandle			m_ShadowPassHandle;
		ptr<TNNAOPass>					m_TNNAOPass;
		vkrg::RenderPassHandle			m_TNNAOPassHandle;
	};
}



