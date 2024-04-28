#include "ShadowPass.h"

namespace kbs
{
	void ShadowPass::OnSceneRender(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd, RenderCamera& camera, ptr<Scene> scene)
	{
		if (m_ShadowCaster.has_value())
		{
			SetTargetCamera(m_LightCamera);
			
			RenderFilter filter;
			filter.flags = RenderPass_Shadow;
			RenderSceneByCamera(filter, cmd);
		}
	}

	void ShadowPass::GetClearValue(uint32_t attachment, VkClearValue& value)
	{
		if (m_ShadowDepthAttachment.idx == attachment)
		{
			value.depthStencil.depth = 1.0f;
			value.depthStencil.stencil = 0;
		}
	}

	void ShadowPass::GetAttachmentStoreLoadOperation(uint32_t attachment, VkAttachmentLoadOp& loadOp, VkAttachmentStoreOp& storeOp, VkAttachmentLoadOp& stencilLoadOp, VkAttachmentStoreOp& stencilStoreOp)
	{
		if (attachment == m_ShadowDepthAttachment.idx)
		{
			stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		}
	}


	opt<mat4> ShadowPass::GetLightVP()
	{
		if (!m_ShadowCaster.has_value())
		{
			return std::nullopt;
		}

		CameraUBO ubo = m_LightCamera.GetCameraUBO();
		return ubo.projection * ubo.view;
	}

	kbs::opt<kbs::Entity> ShadowPass::GetShadowCaster()
	{
		return m_ShadowCaster;
	}

	void ShadowPass::SetTargetScene(ptr<Scene> scene)
	{
		opt<Entity> shadowCaster;
		scene->IterateAllEntitiesWith<LightComponent>(
			[&](Entity e)
			{
				LightComponent lightComp = e.GetComponent<LightComponent>();
				if (lightComp.shadowCaster.castShadow && !shadowCaster.has_value())
				{
					shadowCaster = e;
				}
			}
		);

		m_ShadowCaster = shadowCaster;

		if (shadowCaster.has_value())
		{
			Transform shadowCasterTransform(shadowCaster.value());
			LightComponent shadowCasterLight = shadowCaster->GetComponent<LightComponent>();

			shadowCasterTransform.SetPosition(shadowCasterTransform.GetPosition() -
				shadowCasterTransform.GetFront() * shadowCasterLight.shadowCaster.distance * 0.8f);

			m_LightCamera = RenderCamera
			(
				CameraComponent::OrthogonalCamera
				(
					shadowCasterLight.shadowCaster.distance,
					1e-3,
					shadowCasterLight.shadowCaster.windowWidth,
					shadowCasterLight.shadowCaster.windowHeight
				),
				shadowCasterTransform
			);
		}

		RendererPass::SetTargetScene(scene);
	}

	void ShadowPass::Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc, ptr<gvk::Context> ctx)
	{
		auto [shadowAttachmentName, shadowAttachmentSlice] = desc.RequireAttachment("shadow").value();
		m_ShadowDepthAttachment = pass->AddImageDepthOutput(shadowAttachmentName.c_str(), shadowAttachmentSlice).value();
	}

	
}



