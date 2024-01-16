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

	struct RenderFilter
	{
		RenderPassFlags				flags;
		RenderCameraCullingResult	cullingResult;
	};

	class Renderer
	{
	public:
		Renderer() = default;

		bool Initialize(ptr<kbs::Window> window, RendererCreateInfo& info);
		void Destroy();

		virtual void RenderScene(ptr<Scene> scene);
		VkFormat GetBackBufferFormat();

		RenderAPI GetAPI();

	protected:
		void RenderSceneByCamera(ptr<Scene> scene, RenderCamera& camera, RenderFilter filter, VkCommandBuffer cmd);
		bool InitializeMaterialPipelines();
		bool CompileRenderGraph(std::string& msg);

		virtual vkrg::RenderPassHandle GetMaterialTargetRenderPass(RenderPassFlags flag) = 0;
		virtual bool InitRenderGraph(ptr<kbs::Window> window, ptr<vkrg::RenderGraph> renderGraph) = 0;

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
		ptr<MeshGroup>	GetMeshGroupByComponent(const RenderableComponent& comp);
		Mesh			GetMeshByComponent(const RenderableComponent& comp);
		uint32_t		m_FramebufferIdx;
		
		std::vector<VkSemaphore>	 m_ColorOutputFinish;
		std::vector<VkFence> m_Fences;
	};
}
