#include "Renderer/DeferredRenderer.h"
#include "Core/Event.h"

namespace kbs
{
	vkrg::RenderPassHandle kbs::DeferredRenderer::GetMaterialTargetRenderPass(RenderPassFlags flag)
	{
		if(kbs_contains_flags(flag , RenderPass_Shadow))
		{
			return m_ShadowPassHandle;
		}

		return m_DeferredPassHandle;
	}


	ptr<kbs::DeferredPass> DeferredRenderer::GetDeferredPass()
	{
		return m_DeferredPass;
	}

	bool DeferredRenderer::InitRenderGraph(ptr<kbs::Window> window, ptr<vkrg::RenderGraph> renderGraph)
	{
		vkrg::ResourceInfo info;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		renderGraph->AddGraphResource("normal", info, false);
		renderGraph->AddGraphResource("color", info, false);
		renderGraph->AddGraphResource("material", info, false);

		info.format = GetBackBufferFormat();
		renderGraph->AddGraphResource("backBuffer", info, true, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		info.format = VK_FORMAT_D24_UNORM_S8_UINT;
		renderGraph->AddGraphResource("depth", info, false);

		info.ext.fixed.x = 1024;
		info.ext.fixed.y = 1024;
		info.ext.fixed.z = 1;
		info.extType = vkrg::ResourceExtensionType::Fixed;
		renderGraph->AddGraphResource("shadow", info, false);

		{
			vkrg::ImageSlice slice;
			slice.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			slice.baseArrayLayer = 0;
			slice.baseMipLevel = 0;
			slice.layerCount = 1;
			slice.levelCount = 1;

			RendererAttachmentDescriptor attachmentDesc;
			attachmentDesc.AddAttachment("color", "color", slice);
			attachmentDesc.AddAttachment("normal", "normal", slice);
			attachmentDesc.AddAttachment("material", "material", slice);

			slice.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			attachmentDesc.AddAttachment("depth", "depth", slice);

			m_DeferredPass = CreateRendererPass<DeferredPass>(attachmentDesc, "deferredPass");
			m_DeferredPassHandle = m_DeferredPass->GetRenderPassHandle();
		}

		{
			vkrg::ImageSlice slice;			
			slice.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			slice.baseArrayLayer = 0;
			slice.baseMipLevel = 0;
			slice.layerCount = 1;
			slice.levelCount = 1;

			RendererAttachmentDescriptor attachmentDesc;
			attachmentDesc.AddAttachment("shadow", "shadow", slice);

			vkrg::RenderPassExtension ext;
			ext.extension.fixed.x = 1024;
			ext.extension.fixed.y = 1024;
			ext.extension.fixed.z = 1;
			ext.extensionType = vkrg::ResourceExtensionType::Fixed;

			m_ShadowPass = CreateRendererPass<ShadowPass>(attachmentDesc, "shadow", ext);
			m_ShadowPassHandle = m_ShadowPass->GetRenderPassHandle();
		}

		{
			vkrg::ImageSlice slice;
			slice.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			slice.baseArrayLayer = 0;
			slice.baseMipLevel = 0;
			slice.layerCount = 1;
			slice.levelCount = 1;

			RendererAttachmentDescriptor attachmentDesc;
			attachmentDesc.AddAttachment("color", "color", slice);
			attachmentDesc.AddAttachment("normal", "normal", slice);
			attachmentDesc.AddAttachment("material", "material", slice);

			slice.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			attachmentDesc.AddAttachment("depth", "depth", slice);
			slice.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			attachmentDesc.AddAttachment("shadow", "shadow", slice);

			slice.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			attachmentDesc.AddAttachment("colorOut", "backBuffer", slice);

			m_DeferredShadingPass = CreateComputePass<DeferredShadingPass>(attachmentDesc, "deferredShadingPass");
			m_DeferredShadingPassHandle = m_DeferredShadingPass->GetRenderPassHandle();
		}


		std::string msg;
		if (!CompileRenderGraph(msg))
		{
			KBS_WARN("fail to initilaize renderere {}", msg.c_str());
			return false;
		}

		auto backBuffers = m_Context->GetBackBuffers();
		auto extData = renderGraph->GetExternalDataFrame();
		for (uint32_t i = 0; i < backBuffers.size(); i++)
		{
			extData.BindImage("backBuffer", i, backBuffers[i]);
		}

		Singleton::GetInstance<EventManager>()->AddListener<WindowResizeEvent>(
			[&](const WindowResizeEvent& e)
			{
				m_DeferredShadingPass->ResizeScreen(e.GetWidth(), e.GetHeight());
			}
		);

		return true;
	}

	void DeferredRenderer::OnSceneRender(ptr<Scene> scene)
	{
		Entity mainCamera = scene->GetMainCamera();


		m_DeferredPass->SetTargetCamera(mainCamera);
		m_DeferredPass->SetTargetScene(scene);

		m_ShadowPass->SetTargetScene(scene);
		m_ShadowPass->SetTargetCamera(mainCamera);

		m_DeferredShadingPass->AttachScene(scene, m_ShadowPass->GetShadowCaster(), m_ShadowPass->GetLightVP());
	}

}


