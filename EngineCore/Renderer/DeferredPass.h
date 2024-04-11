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

	class DeferredShadingPass : public RendererPass
	{
	public:

	protected:

	};
}

