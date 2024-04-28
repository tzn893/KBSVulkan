#include "PTRenderer.h"
#include "Core/Singleton.h"
#include "Asset/AssetManager.h"
#include "Core/Event.h"

struct PTMaterial
{
	vec4 albedo;
	vec4 emission;
	vec4 extinction;

	float metallic;
	float roughness;
	float subsurface;
	float specularTint;

	float sheen;
	float sheenTint;
	float clearcoat;
	float clearcoatGloss;

	float transmission;
	float ior;
	float atDistance;
	float __padding;

	int albedoTexID;
	int metallicRoughnessTexID;
	int normalmapTexID;
	int heightmapTexID;
};

struct PTLight
{
	kbs::vec3 position{};
	float area{};
	kbs::vec3 emission{};
	int type{};
	kbs::vec3 u{};
	float radius{};
	kbs::vec3 v{};
	float __padding;
};

struct PTObjDesc
{
	uint64_t vertexBufferAddr;
	uint64_t indiceBufferAddr;
	uint64_t materialSetIndex;
	int indexPrimitiveOffset;
	int vertexOffset;
};

struct PTUniform
{
	kbs::mat4 view;
	kbs::mat4 proj;
	kbs::vec3 cameraPos;
	uint lights;
	bool doubleSidedLight;
	uint spp;
	uint maxDepth;
	uint frame;
	float aperture;
	float focalDistance;
	//float hdrMultiplier;
	//float hdrResolution;
	float AORayLength;
	int integratorType;
};

void kbs::PTRenderer::SetSPP(uint32_t spp)
{
	m_Spp = spp;
}

void kbs::PTRenderer::SetMaxDepth(uint32_t maxDepth)
{
	m_MaxDepth = maxDepth;
}


void kbs::PTRenderer::SetAperture(float aperture)
{
	m_Aperture = aperture;
}

void kbs::PTRenderer::SetFocalDistance(float focalDistance)
{
	m_FocalDistance = focalDistance;
}

vkrg::RenderPassHandle kbs::PTRenderer::GetMaterialTargetRenderPass(RenderPassFlags flag)
{
	return m_PathTracingPassHandle;
}

bool kbs::PTRenderer::InitRenderGraph(ptr<kbs::Window> window, ptr<vkrg::RenderGraph> renderGraph)
{
	vkrg::ResourceInfo info;
	info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	renderGraph->AddGraphResource("accumulationImage", info, false);

	info.format = GetBackBufferFormat();
	renderGraph->AddGraphResource("backBuffer", info, true, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	kbs::RendererAttachmentDescriptor desc;
	
	vkrg::ImageSlice view;
	view.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view.baseArrayLayer = 0;
	view.layerCount = 1;
	view.baseMipLevel = 0;
	view.levelCount = 1;
	
	desc.AddAttachment("accumulationImage", "accumulationImage", view);
	desc.AddAttachment("backBuffer", "backBuffer", view);
	m_PathTracingPass = CreateRayTracingPass<PathTracingPass>(desc, "pathTracing");
	m_PathTracingPassHandle = m_PathTracingPass->GetRenderPassHandle();

	std::string msg;
	if (!CompileRenderGraph(msg))
	{
		KBS_WARN("fail to initilaize surfel GI renderer {}", msg.c_str());
		return false;
	}

	auto backBuffers = m_Context->GetBackBuffers();
	auto extData = renderGraph->GetExternalDataFrame();
	for (uint32_t i = 0; i < backBuffers.size(); i++)
	{
		extData.BindImage("backBuffer", i, backBuffers[i]);
	}

	Singleton::GetInstance<EventManager>()
		->AddListener<WindowResizeEvent>(
			[&](const WindowResizeEvent& e)
			{
				m_PathTracingPass->SetScreenResolution(e.GetWidth(), e.GetHeight());
			}
	);

	Singleton::GetInstance<EventManager>()
		->AddListener<PTAccumulationUpdateEvent>(
			[&](const PTAccumulationUpdateEvent& _)
			{
				m_AccumulateFrameIdx = 1;
			}
	);

	m_AccumulateFrameIdx = 1;

	return true;
}

void kbs::PTRenderer::OnSceneRender(ptr<Scene> scene)
{
	PTUniform uniform;

	if (GetCurrentFrameIdx() == 0)
	{
		RenderAPI api = GetAPI();
		m_Scene = std::make_shared<RTScene>(scene);

		RTSceneUpdateOption option;
		option.loadDiffuseTex = 1;
		option.loadNormalTex = 1;
		option.loadMetallicRoughnessTex = 1;
		m_Scene->UpdateSceneAccelerationStructure(api, option);

		{
			auto materialSets = m_Scene->GetMaterialSets();

			uint32_t materialBufferSize = materialSets.size() * sizeof(PTMaterial);
			m_MaterialBuffer = api.CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				materialBufferSize, GVK_HOST_WRITE_NONE);

			std::vector<PTMaterial> ptMaterials;
			ptMaterials.reserve(materialSets.size());

			uint32_t i = 0;
			for (const RTMaterialSetDesc& mat : materialSets)
			{
				
				PTMaterial ptMat;
				memcpy(&ptMat.albedo, &mat.pbrParameters, sizeof(PBRMaterialParameter));

				ptMat.albedoTexID = mat.diffuseTexIdx;
				ptMat.normalmapTexID = mat.normalTexIdx;
				ptMat.metallicRoughnessTexID = mat.metallicTexIdx;

				ptMaterials.push_back(ptMat);
			}

			api.UploadBuffer(m_MaterialBuffer, ptMaterials.data(), materialBufferSize);
		}

		{
			std::vector<PTLight> lightDatas;
			scene->IterateAllEntitiesWith<LightComponent>(
				[&](kbs::Entity e)
				{
					LightComponent light = e.GetComponent<LightComponent>();
					Transform lightTransform = Transform(e);
					PTLight lightData;

					lightData.type = light.type;
					lightData.emission = light.intensity;

					if (light.type == LightComponent::Area)
					{
						lightData.u = light.area.u;
						lightData.v = light.area.v;
						lightData.position = lightTransform.GetPosition();
						lightData.area = light.area.area;
					}
					else if (light.type == LightComponent::Directional)
					{
						lightData.position = lightTransform.GetFront();
					}
					else if (light.type == LightComponent::Sphere)
					{
						lightData.area = light.sphere.radius * kbs::pi * 4.0f;
						lightData.radius = light.sphere.radius;
						lightData.position = lightTransform.GetPosition();
					}
					else if (light.type == LightComponent::ConstantEnvoriment)
					{
						lightData.emission = light.intensity;
					}
					lightDatas.push_back(lightData);
				}
			);

			uint32_t lightBufferSize = sizeof(PTLight) * lightDatas.size();

			m_LightBuffer = api.CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				lightBufferSize, GVK_HOST_WRITE_NONE);

			api.UploadBuffer(m_LightBuffer, lightDatas.data(), lightBufferSize);
			m_LightCount = lightDatas.size();
		}

		{
			auto objDescs = m_Scene->GetObjectDescs();
			std::vector<PTObjDesc> objDescDatas;
			for (auto& objDesc : objDescs)
			{
				PTObjDesc objDescData;
				objDescData.indiceBufferAddr = objDesc.indexBufferAddress;
				objDescData.vertexBufferAddr = objDesc.vertexBufferAddress;
				objDescData.materialSetIndex = objDesc.materialSetIndex;
				objDescData.vertexOffset = objDesc.vertexOffset;
				objDescData.indexPrimitiveOffset = objDesc.indexOffset / 3;

				objDescDatas.push_back(objDescData);
			}

			uint32_t objDescBufferSize = objDescDatas.size() * sizeof(PTObjDesc);
			m_ObjectDesc = api.CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				objDescBufferSize, GVK_HOST_WRITE_NONE);

			api.UploadBuffer(m_ObjectDesc, objDescDatas.data(), objDescBufferSize);
		}

		{
			m_UniformBuffer = api.CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(PTUniform), GVK_HOST_WRITE_SEQUENTIAL);
		}

		{
			auto textures = m_Scene->GetTextureGroup();
			ptr<TextureManager> texManager = Singleton::GetInstance<AssetManager>()->GetTextureManager();

			for (uint32_t i = 0; i < textures.size(); i++)
			{
				TextureID tex = textures[i];
				if (auto var = texManager->GetTextureByID(tex); var.has_value())
				{
					m_PathTracingPass->GetRTKernel()->UpdateImageView("TextureSamplers", var.value()->GetMainView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						i, var.value()->GetSampler());
				}
				else
				{
					KBS_ASSERT(false, "this branch should not be reached");
				}
			}
		}

		m_PathTracingPass->GetRTKernel()->UpdateAccelerationStructure("TLAS", m_Scene->GetSceneAccelerationStructure()->GetTlas());
		m_PathTracingPass->GetRTKernel()->UpdateBuffer("Lights", m_LightBuffer);
		m_PathTracingPass->GetRTKernel()->UpdateBuffer("Materials", m_MaterialBuffer);
		m_PathTracingPass->GetRTKernel()->UpdateBuffer("obj", m_ObjectDesc);
		m_PathTracingPass->GetRTKernel()->UpdateBuffer("ubo", m_UniformBuffer);
	}

	// get camera parameters
	{
		RenderCamera renderCamera = RenderCamera(scene->GetMainCamera());
		CameraUBO cameraUBO = renderCamera.GetCameraUBO();

		uniform.view = cameraUBO.view;
		uniform.proj = cameraUBO.projection;
		uniform.cameraPos = cameraUBO.cameraPosition;
	}

	uniform.frame = m_AccumulateFrameIdx++;
	uniform.AORayLength = 0.0f;
	uniform.lights = m_LightCount;
	uniform.integratorType = 0;
	uniform.doubleSidedLight = 1.0f;

	uniform.aperture = m_Aperture;
	uniform.focalDistance = m_FocalDistance;
	uniform.maxDepth = m_MaxDepth;
	uniform.spp = m_Spp;

	m_UniformBuffer->Write(uniform);
	
}


kbs::ptr<kbs::RayTracingKernel> kbs::PathTracingPass::GetRTKernel()
{
	return m_RTKernel;
}


void kbs::PathTracingPass::SetScreenResolution(uint32_t width, uint32_t height)
{
	m_Width = width;
	m_Height = height;
}

void kbs::PathTracingPass::Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc)
{
	auto [accImage, accSlice] = desc.RequireAttachment("accumulationImage").value();
	m_AccumulateImageAttachment = pass->AddImageRTOutput(accImage.c_str(), accSlice, VK_IMAGE_VIEW_TYPE_2D).value();

	auto [backBuffer, backBufferSlice] = desc.RequireAttachment("backBuffer").value();
	m_BackBufferAttachment = pass->AddImageRTOutput(backBuffer.c_str(), backBufferSlice, VK_IMAGE_VIEW_TYPE_2D).value();


	ptr<ShaderManager> shaderManager = Singleton::GetInstance<AssetManager>()->GetShaderManager();
	ShaderID rtShader;

	{
		auto loadedShader = shaderManager->Load("PathTracing/Raytracer/Raytracing.rt");
		KBS_ASSERT(loadedShader.has_value(), "fail to load shader {} for surfel gi renderer", "SurfelGI/surfelPT.rt");
		rtShader = loadedShader.value()->GetShaderID();
	}

	RenderAPI api = GetRenderAPI();
	{
		auto kernel = api.CreateRTKernel(rtShader);
		KBS_ASSERT(kernel.has_value(), "fail to create kernel for shader {} in surfel gi renderer", "SurfelGI/surfelPT.rt");
		m_RTKernel = kernel.value();
	}
}

void kbs::PathTracingPass::OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd)
{
	if (ctx.CheckAttachmentDirtyFlag(m_AccumulateImageAttachment))
	{
		VkImageView accImage = ctx.GetImageAttachment(m_AccumulateImageAttachment);
		m_RTKernel->UpdateImageView("AccumulationImage", accImage, VK_IMAGE_LAYOUT_GENERAL, {}, {});
	}
	VkImageView backBufferImage = ctx.GetImageAttachment(m_BackBufferAttachment);
	m_RTKernel->UpdateImageView("OutputImage", backBufferImage, VK_IMAGE_LAYOUT_GENERAL, {}, {});

	m_RTKernel->Dispatch(m_Width, m_Height, 1, cmd);
}
