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

	class ShadowPass : public RendererPass
	{
	public:

		virtual void OnSceneRender(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd,
			RenderCamera& camera, ptr<Scene> scene) override;

		virtual void GetClearValue(uint32_t attachment, VkClearValue& value) override;

		virtual void GetAttachmentStoreLoadOperation(uint32_t attachment, VkAttachmentLoadOp& loadOp, VkAttachmentStoreOp& storeOp,
			VkAttachmentLoadOp& stencilLoadOp, VkAttachmentStoreOp& stencilStoreOp) override;

		opt<mat4>	GetLightVP();
		opt<Entity> GetShadowCaster();

		virtual void SetTargetScene(ptr<Scene> scene) override;

	protected:
		virtual void Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc,
			ptr<gvk::Context> ctx) override;

		vkrg::RenderPassAttachment m_ShadowDepthAttachment;
		RenderCamera	m_LightCamera;
		opt<Entity>		m_ShadowCaster;
	};
}