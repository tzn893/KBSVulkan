#pragma once
// implementation of https://advances.realtimerendering.com/s2021/SIGGRAPH%20Advances%202021%20-%20Surfel%20GI.pdf
#include "Renderer/Renderer.h"
#include "Renderer/DeferredPass.h"
#include "Renderer/RTScene.h"


namespace kbs
{

	

	class ClearAccelerationStructurePass : public ComputePass
	{
	public:
		ptr<ComputeKernel> GetComputeKernel() 
		{ 
			return m_ClearAccelerationStructKernel; 
		}
	
	protected:
		virtual void	Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc) override;
		virtual void	OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) override;

		ptr<ComputeKernel>	   m_ClearAccelerationStructKernel;
		vkrg::RenderPassAttachment m_SurfelASAttachment;
	};

	class CullUnusedSurfelPass : public ComputePass
	{
	public:
		ptr<ComputeKernel> GetComputeKernel() 
		{
			return m_CullUnusedSurfelKernel;
		}
	protected:
		virtual void	Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc) override;
		virtual void	OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) override;

		ptr<ComputeKernel>	   m_CullUnusedSurfelKernel;

		vkrg::RenderPassAttachment m_SurfelBufferAttachment;
		vkrg::RenderPassAttachment m_SurfelIndexBufferAttachment;
		vkrg::RenderPassAttachment m_SurfelASAttachment;
	};

	class SurfelPathTracingPass : public RayTracingPass
	{
	public:
		ptr<RayTracingKernel> GetRayTracingKernel()
		{
			return m_Kernel;
		}

		void AttachScene(ptr<RTScene> scene);

	private:
		virtual void	Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc) override;
		virtual void	OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) override;

		ptr<RayTracingKernel> m_Kernel;

		vkrg::RenderPassAttachment m_SurfelIndexAttachment;
		vkrg::RenderPassAttachment m_SurfelAttachment;

		ptr<RTScene>		  m_RTScene;
	};

	class InsertSurfelPass : public ComputePass
	{
	public:
		ptr<ComputeKernel> GetComputeKernel()
		{
			return m_InsertSurfelsKernel;
		}

		void SetSampler(VkSampler sampler);
		void SetScreenResolution(uint32_t width, uint32_t height);

	protected:
		virtual void	Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc) override;
		virtual void	OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) override;

		ptr<ComputeKernel>	   m_InsertSurfelsKernel;

		vkrg::RenderPassAttachment m_SurfelASAttachment;
		vkrg::RenderPassAttachment m_SurfelBufferAttachment;
		vkrg::RenderPassAttachment m_SurfelIndexBufferAttachment;

		vkrg::RenderPassAttachment m_DepthAttachment;
		vkrg::RenderPassAttachment m_NormalAttachment;

		VkSampler m_Sampler;
		uint32_t screenWidth = 0;
		uint32_t screenHeight = 0;
	};

	class VisualizeCellPass : public ComputePass
	{
	public:
		ptr<ComputeKernel> GetComputeKernel()
		{
			return m_VisualizeCellShaderKernel;
		}

		void SetSampler(VkSampler sampler);
		void SetScreenResolution(uint32_t width, uint32_t height);

	protected:
		virtual void	Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc) override;
		virtual void	OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) override;

		ptr<ComputeKernel>	   m_VisualizeCellShaderKernel;

		vkrg::RenderPassAttachment m_SurfelASAttachment;
		vkrg::RenderPassAttachment m_SurfelBufferAttachment;

		vkrg::RenderPassAttachment m_DepthAttachment;
		vkrg::RenderPassAttachment m_BackBufferAttachment;
		vkrg::RenderPassAttachment m_NormalAttachment;
		VkSampler m_Sampler;

		uint32_t screenWidth = 0;
		uint32_t screenHeight = 0;
	};

	class BuildAccelerationStructurePass : public ComputePass
	{
	public:
		ptr<ComputeKernel> GetComputeKernel()
		{
			return m_BuildASKernel;
		}

	protected:
		virtual void	Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc) override;
		virtual void	OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) override;

		ptr<ComputeKernel>	   m_BuildASKernel;

		vkrg::RenderPassAttachment m_SurfelASAttachment;
		vkrg::RenderPassAttachment m_SurfelBufferAttachment;
		vkrg::RenderPassAttachment m_SurfelIndexBufferAttachment;
	};

	class SurfelGIRenderer : public Renderer
	{
	public:
		ptr<DeferredPass> GetDeferredPass();

	private:
		void ResizeScreen(uint32_t width, uint32_t height);
		
		virtual vkrg::RenderPassHandle GetMaterialTargetRenderPass(RenderPassFlags flag) override;
		virtual bool InitRenderGraph(ptr<kbs::Window> window, ptr<vkrg::RenderGraph> renderGraph) override;
		virtual void OnSceneRender(ptr<Scene> scene) override;

		ptr<RTScene>		   m_RTScene;

		ptr<RenderBuffer>	   m_SurfelBuffer;
		ptr<RenderBuffer>	   m_SurfelIndexBuffer;
		ptr<RenderBuffer>	   m_SurfelGlobalUniform;

		ptr<RenderBuffer>	   m_SurfelPTObjectDesc;
		ptr<RenderBuffer>	   m_SurfelPTMaterialDesc;
		ptr<RenderBuffer>	   m_SurfelPTLightBuffer;
		ptr<RenderBuffer>	   m_SurfelPTGlobalUniform;

		ptr<SurfelPathTracingPass>	m_SurfelPTPass;
		vkrg::RenderPassHandle		m_SurfelPTPassHandle;

		vkrg::RenderPassHandle m_DeferredPassHandle;
		ptr<DeferredPass>	   m_DeferredPass;

		vkrg::RenderPassHandle					m_ClearASPassHandle;
		ptr<ClearAccelerationStructurePass>		m_ClearASPass;

		vkrg::RenderPassHandle					m_CullUnusedSurfelsPassHandle;
		ptr<CullUnusedSurfelPass>				m_CullUnusedSurfelsPass;

		vkrg::RenderPassHandle					m_InsertSurfelsPassHandle;
		ptr<InsertSurfelPass>					m_InsertSurfelsPass;
		
		vkrg::RenderPassHandle					m_VisualizeSurfelsPassHandle;
		ptr<VisualizeCellPass>					m_VisualizeSurfelsPass;

		vkrg::RenderPassHandle					m_BuildASPassHandle;
		ptr<BuildAccelerationStructurePass>		m_BuildASPass;

		uint32_t								m_ScreenWidth;
		uint32_t								m_ScreenHeight;
		uint32_t								m_FrameCounter = 0;
		uint32_t								m_LightCount;
	};

}