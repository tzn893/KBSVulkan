#pragma once
#include "Renderer/Renderer.h"
#include "Renderer/RTScene.h"
#include "Core/Event.h"

namespace kbs
{

	enum class PTRendererEvent
	{
		PT_AccumulationUpdateEvent = 0x10001
	};

	class PTAccumulationUpdateEvent : public Event
	{
	public:
		PTAccumulationUpdateEvent(){}
		std::string ToString() const override
		{
			return "path tracer accumulation update";
		}
		
		EVENT_CLASS(PTAccumulationUpdateEvent, PTRendererEvent::PT_AccumulationUpdateEvent, EventCategoryApplication)
	};



	class PathTracingPass : public RayTracingPass
	{
	public:

		ptr<RayTracingKernel> GetRTKernel();

		void SetScreenResolution(uint32_t width, uint32_t height);

	private:
		virtual void	Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc) override;
		virtual void	OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) override;

		ptr<RayTracingKernel> m_RTKernel;

		vkrg::RenderPassAttachment m_AccumulateImageAttachment;
		vkrg::RenderPassAttachment m_BackBufferAttachment;

		uint32_t m_Width, m_Height;
	};


	class PTRenderer : public Renderer
	{
	public:

		void SetSPP(uint32_t spp);
		void SetMaxDepth(uint32_t spp);
		void SetAperture(float spp);
		void SetFocalDistance(float spp);

	private:
		virtual vkrg::RenderPassHandle GetMaterialTargetRenderPass(RenderPassFlags flag) override;
		virtual bool InitRenderGraph(ptr<kbs::Window> window, ptr<vkrg::RenderGraph> renderGraph) override;
		virtual void OnSceneRender(ptr<Scene> scene) override;
		
		ptr<RTScene> m_Scene;

		vkrg::RenderPassHandle	m_PathTracingPassHandle;
		ptr<PathTracingPass>	m_PathTracingPass;

		ptr<RenderBuffer>		m_MaterialBuffer;
		ptr<RenderBuffer>		m_LightBuffer;
		ptr<RenderBuffer>		m_ObjectDesc;
		ptr<RenderBuffer>		m_UniformBuffer;

		uint32_t				m_LightCount;
		uint32_t				m_Spp = 1;
		uint32_t				m_MaxDepth = 3;
		float					m_Aperture = 0.0001;
		float					m_FocalDistance = 1.0;

		uint32_t				m_AccumulateFrameIdx;
	};
}