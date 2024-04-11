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
	Entity					cubeEntity;
	kbs::vec3				rotateAxis = kbs::math::normalize(kbs::vec3(1, 1, 1));
	Angle					angle = 0;
	MaterialID				cubeMaterial;
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
{
	// 立方体的8个顶点
	vec3 positions[] = {
		vec3(-1.0f, -1.0f,  1.0f),
		vec3(1.0f, -1.0f,  1.0f),
		vec3(1.0f,  1.0f,  1.0f),
		vec3(-1.0f,  1.0f,  1.0f),
		vec3(-1.0f, -1.0f, -1.0f),
		vec3(1.0f, -1.0f, -1.0f),
		vec3(1.0f,  1.0f, -1.0f),
		vec3(-1.0f,  1.0f, -1.0f)
	};

	// 立方体的6个面的法向量
	vec3 normals[] = {
		vec3(0.0f,  0.0f,  1.0f),
		vec3(1.0f,  0.0f,  0.0f),
		vec3(0.0f,  0.0f, -1.0f),
		vec3(-1.0f,  0.0f,  0.0f),
		vec3(0.0f,  1.0f,  0.0f),
		vec3(0.0f, -1.0f,  0.0f)
	};

	// 立方体的顶点
	ShaderStandardVertex cube[] = {
		// 前面
		{positions[0], normals[0], vec2(0.0f, 0.0f), vec3(0.0f)},
		{positions[1], normals[0], vec2(1.0f, 0.0f), vec3(0.0f)},
		{positions[2], normals[0], vec2(1.0f, 1.0f), vec3(0.0f)},
		{positions[0], normals[0], vec2(0.0f, 0.0f), vec3(0.0f)},
		{positions[2], normals[0], vec2(1.0f, 1.0f), vec3(0.0f)},
		{positions[3], normals[0], vec2(0.0f, 1.0f), vec3(0.0f)},
		// 右面
		{positions[1], normals[1], vec2(0.0f, 0.0f), vec3(0.0f)},
		{positions[5], normals[1], vec2(1.0f, 0.0f), vec3(0.0f)},
		{positions[6], normals[1], vec2(1.0f, 1.0f), vec3(0.0f)},
		{positions[1], normals[1], vec2(0.0f, 0.0f), vec3(0.0f)},
		{positions[6], normals[1], vec2(1.0f, 1.0f), vec3(0.0f)},
		{positions[2], normals[1], vec2(0.0f, 1.0f), vec3(0.0f)},
		// 后面
		{positions[5], normals[2], vec2(0.0f, 0.0f), vec3(0.0f)},
		{positions[4], normals[2], vec2(1.0f, 0.0f), vec3(0.0f)},
		{positions[7], normals[2], vec2(1.0f, 1.0f), vec3(0.0f)},
		{positions[5], normals[2], vec2(0.0f, 0.0f), vec3(0.0f)},
		{positions[7], normals[2], vec2(1.0f, 1.0f), vec3(0.0f)},
		{positions[6], normals[2], vec2(0.0f, 1.0f), vec3(0.0f)},
		// 左面
		{positions[4], normals[3], vec2(0.0f, 0.0f), vec3(0.0f)},
		{positions[0], normals[3], vec2(1.0f, 0.0f), vec3(0.0f)},
		{positions[3], normals[3], vec2(1.0f, 1.0f), vec3(0.0f)},
		{positions[4], normals[3], vec2(0.0f, 0.0f), vec3(0.0f)},
		{positions[3], normals[3], vec2(1.0f, 1.0f), vec3(0.0f)},
		{positions[7], normals[3], vec2(0.0f, 1.0f), vec3(0.0f)},
		// 上面
		{positions[3], normals[4], vec2(0.0f, 0.0f), vec3(0.0f)},
		{positions[2], normals[4], vec2(1.0f, 0.0f), vec3(0.0f)},
		{positions[6], normals[4], vec2(1.0f, 1.0f), vec3(0.0f)},
		{positions[3], normals[4], vec2(0.0f, 0.0f), vec3(0.0f)},
		{positions[6], normals[4], vec2(1.0f, 1.0f), vec3(0.0f)},
		{positions[7], normals[4], vec2(0.0f, 1.0f), vec3(0.0f)},
		// 下面
		{positions[0], normals[5], vec2(0.0f, 0.0f), vec3(0.0f)},
		{positions[4], normals[5], vec2(1.0f, 0.0f), vec3(0.0f)},
		{positions[5], normals[5], vec2(1.0f, 1.0f), vec3(0.0f)},
		{positions[0], normals[5], vec2(0.0f, 0.0f), vec3(0.0f)},
		{positions[5], normals[5], vec2(1.0f, 1.0f), vec3(0.0f)},
		{positions[1], normals[5], vec2(0.0f, 1.0f), vec3(0.0f)}
	};

}


void RotatingCubeApplication::BeforeRun()
{
	renderer = std::make_shared<ComplexSceneRenderer>();
	scene = std::make_shared<Scene>();

	RendererCreateInfo info;
	info.instance.AddInstanceExtension(GVK_INSTANCE_EXTENSION_DEBUG);
	info.instance.AddLayer(GVK_LAYER_DEBUG);
	info.instance.AddLayer(GVK_LAYER_FPS_MONITOR);

	Singleton::GetInstance<FileSystem<ShaderManager>>()->AddSearchPath(ROTATING_CUBE_ROOT_DIRECTORY);
	
	renderer->Initialize(m_Window, info);
	
	RenderAPI api = renderer->GetAPI();

	Singleton::GetInstance<AssetManager>()->GetTextureManager()->LoadDefaultTextures(api);
	ptr<RenderBuffer> cubeMeshBuffer = api.CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(cube), GVK_HOST_WRITE_SEQUENTIAL);
	cubeMeshBuffer->GetBuffer()->Write(cube, 0, sizeof(cube));

	MeshGroupID groupId = Singleton::GetInstance<AssetManager>()->GetMeshPool()->CreateMeshGroup(cubeMeshBuffer, _countof(cube));
	MeshID meshId = Singleton::GetInstance<AssetManager>()->GetMeshPool()->CreateMeshFromGroup(groupId, 0, _countof(cube), 0, 0);

	scene = std::make_shared<Scene>();
	// add cube to scene
	{
		Entity entity = scene->CreateEntity("cube0");

		RenderableComponent render{};
		render.targetMesh = meshId;
		ptr<GraphicsShader> shader = std::dynamic_pointer_cast<GraphicsShader>(Singleton::GetInstance<AssetManager>()->GetShaderManager()->Load("shade.glsl").value());

		MaterialID materialId = Singleton::GetInstance<AssetManager>()->GetMaterialManager()->CreateMaterial(shader, api, "m1");
		render.AddRenderablePass(materialId, 0);

		TransformComponent comp(kbs::vec3(0, 0, 8), kbs::math::axisAngle(rotateAxis, angle), kbs::vec3(2, 2, 2));
		comp.parent = scene->GetRootID();

		entity.AddComponent<TransformComponent>(comp);
		entity.AddComponent<RenderableComponent>(render);

		cubeEntity = entity;
		cubeMaterial = materialId;

		Entity entity2 = scene->CreateEntity("cube2");
		entity2.AddComponent<RenderableComponent>(render);


		comp.position = kbs::vec3(1.1, -1.1, 1.1);
		comp.scale = kbs::vec3(0.1, 0.1, 0.1);
		comp.parent = entity.GetUUID();
		entity2.AddComponent<TransformComponent>(comp);
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
		TransformComponent& comp = cubeEntity.GetComponent<TransformComponent>();
		angle = Angle::FromDegree(1234.f * m_Timer->TotalTime());
		comp.rotation = kbs::math::axisAngle(rotateAxis, angle);
	}
	
	{
		angle = Angle::FromDegree(100.f * m_Timer->TotalTime());
		TransformComponent& comp = mainCamera.GetComponent<TransformComponent>();
		Transform cameraTrans(mainCamera.GetComponent<TransformComponent>(), mainCamera);
		//cameraTrans.SetLocalRotation(kbs::math::axisAngle(kbs::vec3(1, 1, 1), angle));
		//cameraTrans.SetLocalPosition(kbs::vec3(0, 0, math::sin(angle)));
		comp = cameraTrans.GetComponent();
	}
	
	ptr<Material> mat = Singleton::GetInstance<AssetManager>()->GetMaterialManager()->GetMaterialByID(cubeMaterial).value();
	
	mat->SetFloat("time", kbs::math::sin(Angle::FromDegree(m_Timer->TotalTime() * 90.f)) * 0.5 + 0.5);
	mat->SetVec3("baseColor", kbs::vec3(0.3, 0., 0.));

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
