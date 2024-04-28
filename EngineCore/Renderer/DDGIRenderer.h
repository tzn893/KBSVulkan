#pragma once
#include "Renderer/Renderer.h"
#include "Renderer/RTScene.h"
#include "Core/Event.h"

#include "Renderer/DeferredPass.h"
#include "Renderer/ShadowPass.h"


namespace kbs
{
	class DDGIRenderer : public kbs::Renderer
	{
	public:
		DDGIRenderer() = default;

		ptr<DeferredPass> GetDeferredPass();

	protected:
		virtual vkrg::RenderPassHandle GetMaterialTargetRenderPass(RenderPassFlags flag) override;
		virtual bool InitRenderGraph(ptr<kbs::Window> window, ptr<vkrg::RenderGraph> renderGraph) override;

		virtual void OnSceneRender(ptr<Scene> scene) override;

		ptr<DeferredPass>				m_DeferredPass;
		vkrg::RenderPassHandle			m_DeferredPassHandle;
		ptr<DeferredShadingPass>		m_DeferredShadingPass;
		vkrg::RenderPassHandle			m_DeferredShadingPassHandle;
		ptr<ShadowPass>					m_ShadowPass;
		vkrg::RenderPassHandle			m_ShadowPassHandle;
	};
}