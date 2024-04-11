#pragma once
#include "Common.h"
#include "Core/Window.h"
#include "vkrg/graph.h"
#include "Scene/Scene.h"
#include "Renderer/RenderCamera.h"
#include "Renderer/RenderResource.h"
#include "Renderer/Material.h"
#include "Renderer/Flags.h"
#include "Renderer/Mesh.h"
#include "Renderer/RenderAPI.h"

namespace kbs
{

	struct RendererCreateInfo
	{
		GvkDeviceCreateInfo		device;
		GvkInstanceCreateInfo	instance;
		std::string				appName;
	};

	struct RenderableObject
	{
		UUID id;
		MaterialID          targetMaterial;
		uint64_t            targetRenderFlag;
		MeshID              targetMeshID;
		Transform           transform;
	};

	using RenderableObjectSorter = std::function<void(std::vector<RenderableObject>&)>;
	using RenderShaderFilter = std::function<bool(ShaderID shaderID)>;

	struct RenderFilter
	{
		RenderPassFlags				flags;
		RenderCameraCullingResult	cullingResult;
		RenderableObjectSorter		renderableObjectSorter;
		RenderShaderFilter			shaderFilter;
	};

	class Renderer;

	class RendererAttachmentDescriptor
	{
	public:
		RendererAttachmentDescriptor() = default;
		RendererAttachmentDescriptor(const RendererAttachmentDescriptor&) = default;

		void AddAttachment(std::string attachmentName, std::string attachmentParameterName, vkrg::ImageSlice view)
		{
			attachments[attachmentName] = std::make_tuple(attachmentParameterName, view);
		}

		void AddBufferAttachment(std::string attachmentName, std::string attachmentParameterName, vkrg::BufferSlice slice)
		{
			bufferAttachemnts[attachmentName] = std::make_tuple(attachmentParameterName, slice);
		}

		opt<tpl<std::string, vkrg::ImageSlice>> RequireAttachment(const std::string& name) const
		{
			if (attachments.count(name))
			{
				return attachments.at(name);
			}
			return std::nullopt;
		}

		opt<tpl<std::string, vkrg::BufferSlice>> RequireBufferAttachment(const std::string& name) const
		{
			if (bufferAttachemnts.count(name))
			{
				return bufferAttachemnts.at(name);
			}
			return std::nullopt;
		}
	private:

		std::unordered_map<std::string, tpl<std::string, vkrg::ImageSlice>> attachments;
		std::unordered_map<std::string, tpl<std::string, vkrg::BufferSlice>> bufferAttachemnts;
	};

	class RayTracingPass : public vkrg::RenderPassInterface
	{
	public:
		RayTracingPass() : vkrg::RenderPassInterface(NULL) {}

		ptr<vkrg::RenderPass> InitializePass(Renderer* render, ptr<vkrg::RenderGraph> graph, RendererAttachmentDescriptor& desc, const std::string& name);

		virtual vkrg::RenderPassType ExpectedType() override { return vkrg::RenderPassType::Raytracing; }

		vkrg::RenderPassHandle	GetRenderPassHandle();

		virtual void OnRender(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) override;

	protected:
		virtual void	Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc) = 0;
		RenderAPI		GetRenderAPI();
		virtual void	OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) = 0;

		Renderer* m_Renderer;
		vkrg::RenderPassHandle m_RTPassHandle;
	};

	class ComputePass : public vkrg::RenderPassInterface
	{
	public:
		ComputePass() : vkrg::RenderPassInterface(NULL) {}

		ptr<vkrg::RenderPass> InitializePass(Renderer* render, ptr<vkrg::RenderGraph> graph, RendererAttachmentDescriptor& desc, const std::string& name);

		virtual vkrg::RenderPassType ExpectedType() override { return vkrg::RenderPassType::Compute; }

		vkrg::RenderPassHandle	GetRenderPassHandle();

		virtual void OnRender(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) override;

	protected:
		virtual void	Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc) = 0;
		RenderAPI		GetRenderAPI();
		virtual void	OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) = 0;

		Renderer* m_Renderer;
		vkrg::RenderPassHandle m_ComputePassHandle;
	};

	class RendererPass : public vkrg::RenderPassInterface
	{
	public:
		RendererPass() :vkrg::RenderPassInterface(NULL) {}


		ptr<vkrg::RenderPass> InitializePass(Renderer* renderer, ptr<gvk::Context> ctx,
			ptr<vkrg::RenderGraph> graph, RendererAttachmentDescriptor& desc,
			const std::string& name);

		void SetTargetScene(ptr<Scene> scene);
		void SetTargetCamera(const RenderCamera& camera);

		virtual void OnSceneRender(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd, 
			RenderCamera& camera, ptr<Scene> scene) = 0;

		virtual void OnRender(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) override;
		
		vkrg::RenderPassHandle	GetRenderPassHandle();

		virtual vkrg::RenderPassType ExpectedType() override { return vkrg::RenderPassType::Graphics; }

	protected:

		class PassParameterUpdater
		{
		public:
			PassParameterUpdater() = default;
			PassParameterUpdater(const PassParameterUpdater&) = default;
			PassParameterUpdater(ptr<gvk::Context> ctx, ptr<gvk::DescriptorSet> set, RenderAPI api):
			ctx(ctx), set(set), api(api){}

			void UpdateImage(const std::string& name,VkImageView view, const std::string& samplerName, VkImageLayout layout);
			void UpdateImage(const std::string& name, VkImageView view, VkSampler sampler, VkImageLayout layout);

		private:
			ptr<gvk::Context> ctx;
			ptr<gvk::DescriptorSet> set;
			RenderAPI api;
		};

		virtual void Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc,
			ptr<gvk::Context> ctx) = 0;

		ptr<Scene> GetTargetScene() { return m_TargetScene; }
		RenderCamera GetRenderCamera() { return m_Camera; }
		Renderer* GetRenderer() { return m_Renderer; }

		void RenderSceneByCamera(RenderFilter filter, VkCommandBuffer cmd);

		void SetUpPerpassParameterSet(ShaderID targetShaderID);
		PassParameterUpdater UpdatePassParameter();

		ptr<gvk::Context> m_Context;
		ptr<Scene> m_TargetScene;
		RenderCamera m_Camera;
		Renderer* m_Renderer;
		
		// TODO better way than create a dummy pipeline
		ptr<gvk::DescriptorSet> m_PerdrawDescriptorSet;
		VkPipelineLayout        m_PerdrawLayout;
		ptr<gvk::Pipeline>		m_PerdrawDummyPipeline;

		vkrg::RenderPassHandle  m_RenderPassHandle;
	};

	class Renderer
	{
	public:
		Renderer() = default;

		bool Initialize(ptr<kbs::Window> window, RendererCreateInfo& info);
		void Destroy();

		void RenderScene(ptr<Scene> scene);
		VkFormat GetBackBufferFormat();

		RenderAPI GetAPI();
		RenderableObjectSorter GetDefaultRenderableObjectSorter(vec3 cameraPosistion);
		RenderShaderFilter	   GetDefaultShaderFilter();

		void RenderSceneByCamera(ptr<Scene> scene, RenderCamera& camera, RenderFilter filter, VkCommandBuffer cmd);
	
		uint32_t GetCurrentFrameIdx();

	protected:

		template<typename RendererPassType>
		ptr<RendererPassType> CreateRendererPass(RendererAttachmentDescriptor& desc, const std::string& name)
		{
			static_assert(std::is_base_of_v<RendererPass, RendererPassType>, "RendererPassType must be derived from RendererPass");

			ptr<RendererPassType> rendererPass = std::make_shared<RendererPassType>();
			ptr<vkrg::RenderPass> rgRenderPass = rendererPass->InitializePass(this, m_Context, m_Graph, desc, name);
			rendererPass->AttachRenderPass(rgRenderPass.get());
			rgRenderPass->AttachInterface(rendererPass);

			return rendererPass;
		}


		template<typename ComputePassType>
		ptr<ComputePassType> CreateComputePass(RendererAttachmentDescriptor& desc, const std::string& name)
		{
			static_assert(std::is_base_of_v<ComputePass, ComputePassType>, "RendererPassType must be derived from ComputePass");

			ptr<ComputePassType> rendererPass = std::make_shared<ComputePassType>();
			ptr<vkrg::RenderPass> rgRenderPass = rendererPass->InitializePass(this, m_Graph, desc, name);
			rendererPass->AttachRenderPass(rgRenderPass.get());
			rgRenderPass->AttachInterface(rendererPass);

			return rendererPass;
		}

		template<typename RTPassType>
		ptr<RTPassType> CreateRayTracingPass(RendererAttachmentDescriptor& desc, const std::string& name)
		{
			static_assert(std::is_base_of_v<RayTracingPass, RTPassType>, "RendererPassType must be derived from ComputePass");

			ptr<RTPassType> rendererPass = std::make_shared<RTPassType>();
			ptr<vkrg::RenderPass> rgRenderPass = rendererPass->InitializePass(this, m_Graph, desc, name);
			rendererPass->AttachRenderPass(rgRenderPass.get());
			rgRenderPass->AttachInterface(rendererPass);

			return rendererPass;
		}

		bool InitializeMaterialPipelines();
		bool CompileRenderGraph(std::string& msg);

		virtual vkrg::RenderPassHandle GetMaterialTargetRenderPass(RenderPassFlags flag) = 0;
		virtual bool InitRenderGraph(ptr<kbs::Window> window, ptr<vkrg::RenderGraph> renderGraph) = 0;
		virtual void OnSceneRender(ptr<Scene> scene) {}

		static constexpr uint32_t				m_ObjectPoolSize = 1024;
		static constexpr uint32_t				m_ObjectCameraPoolSize = m_ObjectPoolSize + 1;
		VkDescriptorSetLayout					m_ObjectDescriptorSetLayout;
		VkDescriptorPool						m_ObjectCameraDescriptorPool;
		std::vector<VkDescriptorSet>			m_ObjectDescriptorSetPool;
		ptr<RenderBuffer>						m_ObjectUBOPool;

		// TODO camera buffer and camera descriptor set
		ptr<RenderBuffer>						m_CameraBuffer;
		VkDescriptorSet							m_CameraDescriptorSet;
		VkDescriptorSetLayout					m_CameraDescriptorSetLayout;

		ptr<gvk::DescriptorAllocator>			m_APIDescriptorSetAllocator;
		
		ptr<gvk::DescriptorAllocator>			m_MaterialDescriptorAllocator;
		std::unordered_map<ShaderID, ptr<gvk::Pipeline>>		m_ShaderPipelines;
		std::unordered_map<MaterialID, ptr<gvk::DescriptorSet>> m_MaterialDescriptors;
		
		ptr<vkrg::RenderGraph>	m_Graph;
		ptr<gvk::Context>		m_Context;
		ptr<kbs::Window>		m_Window;
		VkFormat				m_BackBufferFormat;

		std::vector<VkCommandBuffer> m_PrimaryCmdBuffer;
		ptr<gvk::CommandPool>		 m_PrimaryCmdPool;
		ptr<gvk::CommandQueue>		 m_PrimaryCmdQueue;
	private:
		bool InitializeObjectDescriptorPool();

		ptr<Material>	GetMaterialByID(MaterialID id);
		ptr<MeshGroup>	GetMeshGroupByMesh(const MeshID& id);
		Mesh			GetMeshByMeshID(const MeshID& comp);
		uint32_t		m_FramebufferIdx;
		
		std::vector<VkSemaphore>	 m_ColorOutputFinish;
		std::vector<VkFence> m_Fences;

		uint32_t		m_FrameCounter;
	};
}
