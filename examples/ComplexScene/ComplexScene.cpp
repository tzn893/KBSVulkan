#include "KBS.h"
#include "Core/Entry.h"
#include <iostream>

using namespace kbs;


class ComplexSceneRenderer : public kbs::Renderer
{
public:
	ComplexSceneRenderer() = default;
protected:
	virtual vkrg::RenderPassHandle GetMaterialTargetRenderPass(RenderPassFlags flag) override;
	virtual bool InitRenderGraph(ptr<kbs::Window> window, ptr<vkrg::RenderGraph> renderGraph) override;

	vkrg::RenderPassHandle	mainRenderPass;
	friend class ComplexSceneMainPass;
};

class ComplexSceneMainPass : public vkrg::RenderPassInterface
{
public:
	ComplexSceneMainPass(vkrg::RenderPass* pass, vkrg::RenderPassAttachment backBuffer, vkrg::RenderPassAttachment depthBuffer, ComplexSceneRenderer* renderer)
		:vkrg::RenderPassInterface(pass), backBuffer(backBuffer), depthBuffer(depthBuffer), renderer(renderer)
	{

	}

	virtual void GetClearValue(uint32_t attachment, VkClearValue& value)
	{
		if (attachment == depthBuffer.idx)
		{
			value.depthStencil.depth = 1;
			value.depthStencil.stencil = 0;
		}
	}

	virtual void GetAttachmentStoreLoadOperation(uint32_t attachment, VkAttachmentLoadOp& loadOp, VkAttachmentStoreOp& storeOp,
		VkAttachmentLoadOp& stencilLoadOp, VkAttachmentStoreOp& stencilStoreOp)
	{
		if (attachment == depthBuffer.idx)
		{
			loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
		if (attachment == backBuffer.idx)
		{
			loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
	}

	virtual void OnRender(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd) override;

	virtual vkrg::RenderPassType ExpectedType()
	{
		return vkrg::RenderPassType::Graphics;
	}
private:
	vkrg::RenderPassAttachment backBuffer;
	vkrg::RenderPassAttachment depthBuffer;
	ComplexSceneRenderer* renderer;
};

class RotatingCubeApplication : public kbs::Application
{
public:

	RotatingCubeApplication(kbs::ApplicationCommandLine& commandLine)
		: Application(commandLine){}

	virtual void OnUpdate() override;
	
	virtual void BeforeRun() override;
private:
	Entity					modelEntity;
	kbs::vec3				rotateAxis = kbs::math::normalize(kbs::vec3(1, 1, 1));
	Angle					angle = 0;
};


namespace kbs
{
	Application* CreateApplication(ApplicationCommandLine& commandLines)
	{
		auto app = new RotatingCubeApplication(commandLines);
		return app;
	}
}

ptr<Scene> scene;
ptr<ComplexSceneRenderer> renderer;
Entity					  mainCamera;


namespace kbs
{}


void RotatingCubeApplication::BeforeRun()
{
	renderer = std::make_shared<ComplexSceneRenderer>();
	scene = std::make_shared<Scene>();

	Singleton::GetInstance<FileSystem<ShaderManager>>()->AddSearchPath(COMPLEX_SCENE_ROOT_DIRECTORY);
	Singleton::GetInstance<FileSystem<ModelManager>>()->AddSearchPath(ASSET_ROOT_DIRECTORY);
	auto assetManager = Singleton::GetInstance<AssetManager>();
	
	RendererCreateInfo info;
	info.instance.AddInstanceExtension(GVK_INSTANCE_EXTENSION_DEBUG);
	info.instance.AddLayer(GVK_LAYER_DEBUG);
	info.instance.AddLayer(GVK_LAYER_FPS_MONITOR);

	renderer->Initialize(m_Window, info);
	
	ptr<GraphicsShader> shader = std::dynamic_pointer_cast<GraphicsShader>(Singleton::GetInstance<AssetManager>()->GetShaderManager()->Load("shade.glsl").value());

	RenderAPI api = renderer->GetAPI();
	scene = std::make_shared<Scene>();

	{
		ModelID modelID = assetManager->GetModelManager()->LoadFromGLTF("models/sponza/sponza.gltf", api).value();
		ptr<Model> model = assetManager->GetModelManager()->GetModel(modelID).value();

		TransformComponent modelTransform;
		modelTransform.parent = scene->GetRootID();
		modelTransform.position = kbs::vec3(0);
		modelTransform.rotation = kbs::quat();
		modelTransform.scale = kbs::vec3(1);

		ptr<ModelMaterialSet> materialSet = model->CreateMaterialSetForModel(shader->GetShaderID(), api).value();
		modelEntity = model->Instantiate(scene, "sponza", materialSet, modelTransform);
	}
	
	// add main camera to scene
	{
		CameraComponent camera(1000, 0.1, 1, kbs::pi / 2);
		TransformComponent trans(kbs::vec3(), kbs::quat(), kbs::vec3(1, 1, 1));
		trans.parent = scene->GetRootID();

		mainCamera = scene->CreateEntity("main camera");
		mainCamera.AddComponent<CameraComponent>(camera);
		mainCamera.AddComponent<TransformComponent>(trans);

		Transform cameraTrans(trans, mainCamera);
		//cameraTrans.SetPosition(kbs::vec3(0, 0, 0));
		mainCamera.GetComponent<TransformComponent>() = cameraTrans.GetComponent();
	}
}

vkrg::RenderPassHandle ComplexSceneRenderer::GetMaterialTargetRenderPass(RenderPassFlags flag)
{
	return mainRenderPass;
}

void RotatingCubeApplication::OnUpdate()
{
	
	{
		angle = Angle::FromDegree(100.f * m_Timer->TotalTime());
		TransformComponent& comp = mainCamera.GetComponent<TransformComponent>();
		Transform cameraTrans(mainCamera.GetComponent<TransformComponent>(), mainCamera);
		cameraTrans.SetLocalRotation(kbs::math::axisAngle(kbs::vec3(0, 1, 0), angle));
		// cameraTrans.SetLocalPosition(kbs::vec3(0, math::sin(angle) + 3, 0));
		comp = cameraTrans.GetComponent();
	}
	
	 renderer->RenderScene(scene);
}


bool ComplexSceneRenderer::InitRenderGraph(ptr<kbs::Window> window, ptr<vkrg::RenderGraph> renderGraph)
{
	const char* backBuffer = "backBuffer";
	const char* mainPass = "mainPass";
	const char* depthBuffer = "depthBuffer";

	vkrg::ResourceInfo info;
	info.format = renderer->GetBackBufferFormat();
	renderGraph->AddGraphResource(backBuffer, info, true, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	info.format = VK_FORMAT_D24_UNORM_S8_UINT;
	renderGraph->AddGraphResource(depthBuffer, info, false, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	{
		mainRenderPass = renderGraph->AddGraphRenderPass(mainPass, vkrg::RenderPassType::Graphics).value();
		vkrg::ImageSlice slice;
		slice.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		slice.baseArrayLayer = 0;
		slice.baseMipLevel = 0;
		slice.layerCount = 1;
		slice.levelCount = 1;

		auto backBufferAttachment = mainRenderPass.pass->AddImageColorOutput(backBuffer, slice).value();
		slice.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		auto depthBufferAttachment = mainRenderPass.pass->AddImageDepthOutput(depthBuffer, slice).value();
		mainRenderPass.pass->AttachInterface(std::make_shared<ComplexSceneMainPass>(mainRenderPass.pass.get(),
			backBufferAttachment, depthBufferAttachment, this));
	}

	std::string msg;
	if (!CompileRenderGraph(msg))
	{
		KBS_WARN("fail to initilaize renderere {}", msg.c_str());
		return false;
	}

	auto backBuffers = m_Context->GetBackBuffers();
	auto extData = renderGraph->GetExternalDataFrame();
	for (uint32_t i = 0;i < backBuffers.size();i++)
	{
		extData.BindImage(backBuffer, i, backBuffers[i]);
	}
	
	return true;
}

void ComplexSceneMainPass::OnRender(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd)
{
	RenderCamera renderCamera(mainCamera);

	RenderFilter filter;
	filter.flags = RenderPass_Opaque;

	renderer->RenderSceneByCamera(scene, renderCamera, filter, cmd);
}
