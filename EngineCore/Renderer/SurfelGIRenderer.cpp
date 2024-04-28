#include "Renderer/SurfelGIRenderer.h"
#include "Core/Singleton.h"
#include "Asset/AssetManager.h"
#include "Core/Event.h"

#include "Renderer/PTRenderer.h"

namespace kbs
{
	static constexpr uint32_t m_MaxSurfelBufferCount = 65535;
	static constexpr uint32_t m_SurfelGridCount = 64;
	static constexpr uint32_t m_SurfelCellIndexPoolCount = 4194304;

	static const char* surfelBufferName = "surfelBuffer";
	static const char* surfelIndexBufferName = "surfelIndexBuffer";
	static const char* surfelAccelerationStructureName = "surfelAS";

	struct SurfelGlobalUniform
	{
		mat4 viewInverse;
		mat4 projInverse;
		vec4 resolution;

		vec3    surfelCellExtent;
		uint    currentFrame;
		vec3    accelerationStructurePosition;
		float   surfelRadius;

		CameraFrustrum	frustrum;
		vec3			position;
	};


	struct SurfelInfo
	{
		vec4		radianceWeight;
		vec3		position;
		float		radius;
		//			the last frame surfel has contribution to main view
		vec3		normal;
		uint32_t	lastContributionFrame;
	};

	struct SurfelAccelerationStructureCell
	{
		int surfels[31];
		int surfelCount;
	};



	struct SurfelPTMaterial
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

	struct SurfelPTLight
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

	struct SurfelPTObjDesc
	{
		uint64_t vertexBufferAddr;
		uint64_t indiceBufferAddr;
		uint64_t materialSetIndex;
		int indexPrimitiveOffset;
		int vertexOffset;
	};

	struct SurfelPTUniform
	{
		kbs::vec3 cameraPos;
		uint lights;
		bool doubleSidedLight;
		uint spp;
		uint maxDepth;
		uint frame;
	};


	void SurfelGIRenderer::ResizeScreen(uint32_t width, uint32_t height)
	{
		m_ScreenWidth = width;
		m_ScreenHeight = height;

		m_VisualizeSurfelsPass->SetScreenResolution(m_ScreenWidth, m_ScreenHeight);
		m_InsertSurfelsPass->SetScreenResolution(m_ScreenWidth, m_ScreenHeight);
	}

	vkrg::ptr<kbs::DeferredPass> SurfelGIRenderer::GetDeferredPass()
	{
		return m_DeferredPass;
	}

	vkrg::RenderPassHandle SurfelGIRenderer::GetMaterialTargetRenderPass(RenderPassFlags flag)
	{
		return m_DeferredPassHandle;
	}

	static void CreateComputeKernel(ptr<ComputeKernel>& targetKernel, const char* shaderPath, RenderAPI api)
	{
		ptr<ShaderManager> shaderManager = Singleton::GetInstance<AssetManager>()->GetShaderManager();
		ShaderID	targetShaderID;
		{
			auto var = shaderManager->Load(shaderPath);
			KBS_ASSERT(var.has_value(), "fail to load shader {} for surfel gi renderer", shaderPath);
			targetShaderID = var.value()->GetShaderID();
		}

		{
			auto var = api.CreateComputeKernel(targetShaderID);
			KBS_ASSERT(var.has_value(), "fail to compute compute kernel for shader {} in surfel gi renderer", shaderPath);
			targetKernel = var.value();
		}
	};

	bool SurfelGIRenderer::InitRenderGraph(ptr<kbs::Window> window, ptr<vkrg::RenderGraph> renderGraph)
	{
		RenderAPI api = GetAPI();

		Singleton::GetInstance<AssetManager>()->GetShaderManager()->GetMacroSet().Define("SURFEL_SPARSE_OPTIMIZED");

		vkrg::ResourceInfo info;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;

		renderGraph->AddGraphResource("color", info, false);
		renderGraph->AddGraphResource("normal", info, false);
		renderGraph->AddGraphResource("material", info, false);

		info.format = VK_FORMAT_D24_UNORM_S8_UINT;
		renderGraph->AddGraphResource("depth", info, false);


		uint32_t surfelBufferSize = sizeof(SurfelInfo) * m_MaxSurfelBufferCount;
		uint32_t indexSurfelBufferSize = (m_MaxSurfelBufferCount + 1) * sizeof(int) * 2;
		uint32_t surfelAccelerationStructureSize = (m_SurfelGridCount * m_SurfelGridCount * m_SurfelGridCount  * 3 + m_SurfelCellIndexPoolCount +  1) * sizeof(int);

		vkrg::ResourceInfo bufferInfo;
		bufferInfo.extType = vkrg::ResourceExtensionType::Buffer;
		bufferInfo.ext.buffer.size = surfelAccelerationStructureSize;
		renderGraph->AddGraphResource(surfelAccelerationStructureName, bufferInfo, false);


		bufferInfo.ext.buffer.size = indexSurfelBufferSize;
		renderGraph->AddGraphResource(surfelIndexBufferName, bufferInfo, true);
		bufferInfo.ext.buffer.size = surfelBufferSize;
		renderGraph->AddGraphResource(surfelBufferName, bufferInfo, true);

		m_SurfelBuffer = api.CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			surfelBufferSize, GVK_HOST_WRITE_NONE);
		m_SurfelIndexBuffer = api.CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			indexSurfelBufferSize, GVK_HOST_WRITE_NONE);
		m_SurfelGlobalUniform = api.CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(SurfelGlobalUniform), GVK_HOST_WRITE_SEQUENTIAL);

		vkrg::ResourceInfo imageInfo;
		imageInfo.format = GetBackBufferFormat();
		renderGraph->AddGraphResource("backBuffer", imageInfo, true, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		vkrg::ImageSlice slice;
		slice.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		slice.baseArrayLayer = 0;
		slice.baseMipLevel = 0;
		slice.layerCount = 1;
		slice.levelCount = 1;

		{
			RendererAttachmentDescriptor attachmentDesc;
			attachmentDesc.AddAttachment("color", "color", slice);
			attachmentDesc.AddAttachment("normal", "normal", slice);
			attachmentDesc.AddAttachment("material", "material", slice);

			slice.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			attachmentDesc.AddAttachment("depth", "depth", slice);

			m_DeferredPass = CreateRendererPass<DeferredPass>(attachmentDesc, "deferredPass");
			m_DeferredPassHandle = m_DeferredPass->GetRenderPassHandle();
		}

		RendererAttachmentDescriptor compAttachmentDesc;
		compAttachmentDesc.AddBufferAttachment("surfelBuffer", "surfelBuffer", vkrg::BufferSlice::fullBuffer);
		compAttachmentDesc.AddBufferAttachment("surfelIndexBuffer", "surfelIndexBuffer", vkrg::BufferSlice::fullBuffer);
		compAttachmentDesc.AddBufferAttachment("surfelAS", "surfelAS", vkrg::BufferSlice::fullBuffer);

		{
			m_ClearASPass = CreateComputePass<ClearAccelerationStructurePass>(compAttachmentDesc, "clearAccelerationStructure");
			m_ClearASPassHandle = m_ClearASPass->GetRenderPassHandle();

			m_ClearASPass->GetComputeKernel()->UpdateBuffer("globalUniform", m_SurfelGlobalUniform);
		}

		{
			m_CullUnusedSurfelsPass = CreateComputePass<CullUnusedSurfelPass>(compAttachmentDesc, "cullUnusedSurfel");
			m_CullUnusedSurfelsPassHandle = m_CullUnusedSurfelsPass->GetRenderPassHandle();

			m_CullUnusedSurfelsPass->GetComputeKernel()->UpdateBuffer("globalUniform", m_SurfelGlobalUniform);
			m_CullUnusedSurfelsPass->GetComputeKernel()->UpdateBuffer("surfels", m_SurfelBuffer);
			m_CullUnusedSurfelsPass->GetComputeKernel()->UpdateBuffer("surfelIdxBuffer", m_SurfelIndexBuffer);
		}


		auto defaultSampler = GetAPI().CreateSampler(GvkSamplerCreateInfo());

		{
			m_InsertSurfelsPass = CreateComputePass<InsertSurfelPass>(compAttachmentDesc, "insertSurfel");
			m_InsertSurfelsPassHandle = m_InsertSurfelsPass->GetRenderPassHandle();

			m_InsertSurfelsPass->GetComputeKernel()->UpdateBuffer("surfels", m_SurfelBuffer);
			m_InsertSurfelsPass->GetComputeKernel()->UpdateBuffer("surfelIdxBuffer", m_SurfelIndexBuffer);
			m_InsertSurfelsPass->GetComputeKernel()->UpdateBuffer("globalUniform", m_SurfelGlobalUniform);

			m_InsertSurfelsPass->SetSampler(defaultSampler.value());
		}

		{
			m_BuildASPass = CreateComputePass<BuildAccelerationStructurePass>(compAttachmentDesc, "buildAccelerationStructurePass");
			m_BuildASPassHandle = m_BuildASPass->GetRenderPassHandle();

			m_BuildASPass->GetComputeKernel()->UpdateBuffer("surfels", m_SurfelBuffer);
			m_BuildASPass->GetComputeKernel()->UpdateBuffer("globalUniform", m_SurfelGlobalUniform);
			m_BuildASPass->GetComputeKernel()->UpdateBuffer("surfelIdxBuffer", m_SurfelIndexBuffer);
		}

		{
			m_VisualizeSurfelsPass = CreateComputePass<VisualizeCellPass>(compAttachmentDesc, "visualizeCellPass");
			m_VisualizeSurfelsPassHandle = m_VisualizeSurfelsPass->GetRenderPassHandle();

			m_VisualizeSurfelsPass->GetComputeKernel()->UpdateBuffer("surfels", m_SurfelBuffer);
			m_VisualizeSurfelsPass->GetComputeKernel()->UpdateBuffer("globalUniform", m_SurfelGlobalUniform);

			m_VisualizeSurfelsPass->SetSampler(defaultSampler.value());
		}

		{
			m_SurfelPTPass = CreateRayTracingPass<SurfelPathTracingPass>(compAttachmentDesc, "surfelPTPass");
			m_SurfelPTPassHandle = m_SurfelPTPass->GetRenderPassHandle();
		}

		renderGraph->AddEdge(m_CullUnusedSurfelsPassHandle, m_BuildASPassHandle);
		renderGraph->AddEdge(m_BuildASPassHandle, m_VisualizeSurfelsPassHandle);
		renderGraph->AddEdge(m_CullUnusedSurfelsPassHandle, m_SurfelPTPassHandle);

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
			extData.BindBuffer("surfelBuffer", i, m_SurfelBuffer->GetBuffer());
			extData.BindBuffer("surfelIndexBuffer", i, m_SurfelIndexBuffer->GetBuffer());
		}

		std::vector<int> indexBufferData((m_MaxSurfelBufferCount + 1) * 2, 0);
		indexBufferData[m_MaxSurfelBufferCount + 1] = m_MaxSurfelBufferCount;
		for (int i = m_MaxSurfelBufferCount + 2, j = 0; i < indexBufferData.size(); i++, j++)
		{
			indexBufferData[i] = j;
		}
		api.UploadBuffer(m_SurfelIndexBuffer, indexBufferData.data(), indexSurfelBufferSize);
		

		Singleton::GetInstance<EventManager>()
			->AddListener<WindowResizeEvent>(
				[&](const WindowResizeEvent& e)
				{
					ResizeScreen(e.GetWidth(), e.GetHeight());
				}
		);

		Singleton::GetInstance<EventManager>()
			->AddListener<PTAccumulationUpdateEvent>(
				[&](const PTAccumulationUpdateEvent& _)
				{
					m_FrameCounter = 1;
				}
		);

		return true;
	}

	void SurfelGIRenderer::OnSceneRender(ptr<Scene> scene)
	{
		Entity mainCamera = scene->GetMainCamera();

		
		if (GetCurrentFrameIdx() == 0)
		{
			m_RTScene = std::make_shared<RTScene>(scene);

			RTSceneUpdateOption option;
			option.loadDiffuseTex = true;
			option.loadNormalTex = true;

			m_RTScene->UpdateSceneAccelerationStructure(GetAPI(), option);

			RenderAPI api = GetAPI();
			
			{
				auto objDesc = m_RTScene->GetObjectDescs();
				uint64_t objectDescSize = sizeof(SurfelPTObjDesc) * objDesc.size();

				m_SurfelPTObjectDesc = api.CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					objectDescSize, GVK_HOST_WRITE_NONE);

				std::vector<SurfelPTObjDesc> surfelRTObjectDesc(objDesc.size());
				for (uint32_t i = 0; i < objDesc.size(); i++)
				{
					surfelRTObjectDesc[i].indiceBufferAddr = objDesc[i].indexBufferAddress;
					surfelRTObjectDesc[i].vertexBufferAddr = objDesc[i].vertexBufferAddress;
					surfelRTObjectDesc[i].materialSetIndex = objDesc[i].materialSetIndex;
					surfelRTObjectDesc[i].vertexOffset = objDesc[i].vertexOffset;
					surfelRTObjectDesc[i].indexPrimitiveOffset = objDesc[i].indexOffset / 3;
				}

				api.UploadBuffer(m_SurfelPTObjectDesc, surfelRTObjectDesc.data(), objectDescSize);
				//m_SurfelPTObjectDesc->Write(surfelRTObjectDesc.data(), objectDescSize);
			}
			
			{
				auto materialDesc = m_RTScene->GetMaterialSets();
				uint64_t materialDescSize = materialDesc.size() * sizeof(SurfelPTMaterial);

				m_SurfelPTMaterialDesc = api.CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					materialDescSize, GVK_HOST_WRITE_NONE);

				std::vector<SurfelPTMaterial> surfelRTMaterialDesc(materialDesc.size());
				for (uint32_t i = 0;i < materialDesc.size(); i++) 
				{
					memcpy(&surfelRTMaterialDesc[i].albedo, &materialDesc[i].pbrParameters, sizeof(materialDesc[i].pbrParameters));
					
					surfelRTMaterialDesc[i].albedoTexID = materialDesc[i].diffuseTexIdx;
					surfelRTMaterialDesc[i].normalmapTexID = materialDesc[i].normalTexIdx;
				}

				api.UploadBuffer(m_SurfelPTMaterialDesc, surfelRTMaterialDesc.data(), materialDescSize);
				//m_SurfelPTMaterialDesc->Write(surfelRTMaterialDesc.data(), materialDescSize);
			}

			{
				bool mainLightFounded = false;
				std::vector<SurfelPTLight> lights;

				scene->IterateAllEntitiesWith<LightComponent>
				(
						[&](Entity e)
						{
							LightComponent lComp = e.GetComponent<LightComponent>();
							Transform trans(e);
				
							if (mainLightFounded)
							{
								return;
							}

							SurfelPTLight lightBuffer;

							lightBuffer.emission = lComp.intensity;
							lightBuffer.type = lComp.type;

							if (lComp.type == LightComponent::LightType::Area)
							{
								lightBuffer.area = lComp.area.area;
								lightBuffer.u = lComp.area.u;
								lightBuffer.v = lComp.area.v;
							}
							else if (lComp.type == LightComponent::LightType::Sphere)
							{
								lightBuffer.radius = lComp.sphere.radius;
								lightBuffer.position = trans.GetPosition();
							}
							else if (lComp.type == LightComponent::LightType::Directional)
							{
								lightBuffer.position = trans.GetFront();
							}

							lights.push_back(lightBuffer);
						}
				);
				
				uint32_t lightBufferSize = sizeof(SurfelPTLight) * lights.size();
				m_SurfelPTLightBuffer = api.CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					lightBufferSize, GVK_HOST_WRITE_RANDOM);
				
				m_LightCount = lights.size();
				m_SurfelPTLightBuffer->Write(lights.data(), lightBufferSize, 0);
			}

			{
				m_SurfelPTGlobalUniform = api.CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(SurfelPTUniform),
					GVK_HOST_WRITE_RANDOM);
			}


			ptr<RayTracingKernel> rtKernel = m_SurfelPTPass->GetRayTracingKernel();
			rtKernel->UpdateBuffer("objDesc", m_SurfelPTObjectDesc);
			rtKernel->UpdateBuffer("Lights", m_SurfelPTLightBuffer);
			rtKernel->UpdateBuffer("materialSets", m_SurfelPTMaterialDesc);
			rtKernel->UpdateBuffer("ubo", m_SurfelPTGlobalUniform);

			{
				auto rtTextures = m_RTScene->GetTextureGroup();
				ptr<TextureManager> texManager = Singleton::GetInstance<AssetManager>()->GetTextureManager();

				for (uint32_t i = 0;i < rtTextures.size();i++)
				{
					TextureID tex = rtTextures[i];
					if (auto var = texManager->GetTextureByID(tex); var.has_value()) 
					{
						rtKernel->UpdateImageView("rtTextures", var.value()->GetMainView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
							i, var.value()->GetSampler());
					}
				}
			}

			rtKernel->UpdateAccelerationStructure("TLAS", m_RTScene->GetSceneAccelerationStructure()->GetTlas());
		}
		RenderCamera mainCameraRenderData(mainCamera);
		Transform mainCameraTransform(mainCamera);

		auto cameraUBO = mainCameraRenderData.GetCameraUBO();

		SurfelGlobalUniform globalUniform;
		globalUniform.projInverse = cameraUBO.invProjection;
		globalUniform.viewInverse = cameraUBO.invView;
		globalUniform.resolution = vec4(m_ScreenWidth, m_ScreenHeight, 1 / (float)m_ScreenWidth, 1 / (float)m_ScreenHeight);

		SurfelPTUniform ptGlobalUniform;
		ptGlobalUniform.cameraPos = cameraUBO.cameraPosition;
		ptGlobalUniform.doubleSidedLight = 1;
		ptGlobalUniform.frame = m_FrameCounter;
		ptGlobalUniform.lights = m_LightCount;
		ptGlobalUniform.maxDepth = 3;

		m_SurfelPTGlobalUniform->Write(ptGlobalUniform);

		vec3 cellExtent = vec3(0.8, 0.8, 0.8);
		vec3 cameraPosition = mainCameraTransform.GetPosition();
		
		vec3 accelerationStructurePoisition;
		//if (frameCounter <= 1)
		accelerationStructurePoisition = cameraPosition - (m_SurfelGridCount / 2.0f) * cellExtent;

		globalUniform.accelerationStructurePosition = accelerationStructurePoisition;
		globalUniform.surfelCellExtent = cellExtent;
		globalUniform.currentFrame = m_FrameCounter;
		globalUniform.surfelRadius = 0.4;

		globalUniform.frustrum = mainCameraRenderData.GetFrustrum();
		globalUniform.position = cameraPosition;

		m_SurfelGlobalUniform->Write(globalUniform);

		m_DeferredPass->SetTargetScene(scene);
		m_DeferredPass->SetTargetCamera(mainCameraRenderData);

		m_FrameCounter++;
	}

	void ClearAccelerationStructurePass::Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc)
	{
		CreateComputeKernel(m_ClearAccelerationStructKernel, "SurfelGI/clearAccelerationStructure.comp", GetRenderAPI());
		auto [surfelASName, surfelASSlice] = desc.RequireBufferAttachment("surfelAS").value();
		m_SurfelASAttachment = pass->AddBufferStorageOutput(surfelASName.c_str(), surfelASSlice).value();
	}

	void ClearAccelerationStructurePass::OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd)
	{
		if (ctx.CheckAttachmentDirtyFlag(m_SurfelASAttachment))
		{
			vkrg::BufferView surfelASAttachment = ctx.GetBufferAttachment(m_SurfelASAttachment);

			m_ClearAccelerationStructKernel->UpdateBuffer("accelStruct", surfelASAttachment.buffer,surfelASAttachment.size,
				surfelASAttachment.offset);
		}

		constexpr uint32_t threadX = m_SurfelGridCount * m_SurfelGridCount * m_SurfelGridCount / 256;
		//if (frameCounter <= 1)
		{
			m_ClearAccelerationStructKernel->Dispatch(threadX, 1, 1, cmd);
		}
	}

	void CullUnusedSurfelPass::Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc)
	{
		CreateComputeKernel(m_CullUnusedSurfelKernel, "SurfelGI/cullUnusedSurfels.comp", GetRenderAPI());
	
		m_SurfelBufferAttachment = pass->AddBufferStorageInput(surfelBufferName, vkrg::BufferSlice::fullBuffer).value();
		m_SurfelIndexBufferAttachment = pass->AddBufferStorageInput(surfelIndexBufferName, vkrg::BufferSlice::fullBuffer).value();
		m_SurfelASAttachment = pass->AddBufferStorageInput(surfelAccelerationStructureName, vkrg::BufferSlice::fullBuffer).value();
	}

	void CullUnusedSurfelPass::OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd)
	{
		if (ctx.CheckAttachmentDirtyFlag(m_SurfelASAttachment))
		{
			vkrg::BufferView view = ctx.GetBufferAttachment(m_SurfelASAttachment);

			m_CullUnusedSurfelKernel->UpdateBuffer("accelStruct", view.buffer, view.size, view.offset);
		}

		constexpr uint32_t threadX = m_MaxSurfelBufferCount / 256 + 1;
		// if (frameCounter <= 1)
		{
			m_CullUnusedSurfelKernel->Dispatch(threadX, 1, 1, cmd);
		}
	}

	void InsertSurfelPass::SetSampler(VkSampler sampler)
	{
		m_Sampler = sampler;
	}

	void InsertSurfelPass::SetScreenResolution(uint32_t width, uint32_t height)
	{
		screenWidth = width;
		screenHeight = height;
	}

	void InsertSurfelPass::Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc)
	{
		CreateComputeKernel(m_InsertSurfelsKernel, "SurfelGI/insertSurfels.comp", GetRenderAPI());

		m_SurfelASAttachment = pass->AddBufferStorageInput(surfelAccelerationStructureName, vkrg::BufferSlice::fullBuffer).value();
		m_SurfelBufferAttachment = pass->AddBufferStorageOutput(surfelBufferName, vkrg::BufferSlice::fullBuffer).value();
		m_SurfelIndexBufferAttachment = pass->AddBufferStorageOutput(surfelIndexBufferName, vkrg::BufferSlice::fullBuffer).value();

		vkrg::ImageSlice slice{};
		slice.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		slice.baseArrayLayer = 0;
		slice.baseMipLevel = 0;
		slice.layerCount = 1;
		slice.levelCount = 1;
		m_DepthAttachment = pass->AddImageColorInput("depth", slice, VK_IMAGE_VIEW_TYPE_2D).value();
		
		slice.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		m_NormalAttachment = pass->AddImageColorInput("normal", slice, VK_IMAGE_VIEW_TYPE_2D).value();
	}

	void InsertSurfelPass::OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd)
	{
		if (ctx.CheckAttachmentDirtyFlag(m_NormalAttachment) || ctx.CheckAttachmentDirtyFlag(m_DepthAttachment)
			|| ctx.CheckAttachmentDirtyFlag(m_SurfelASAttachment))
		{
			vkrg::BufferView asBufferView = ctx.GetBufferAttachment(m_SurfelASAttachment);
			m_InsertSurfelsKernel->UpdateBuffer("accelStruct", asBufferView.buffer, asBufferView.size, asBufferView.offset);

			VkImageView normal = ctx.GetImageAttachment(m_NormalAttachment);
			VkImageView depth = ctx.GetImageAttachment(m_DepthAttachment);
			m_InsertSurfelsKernel->UpdateImageView("gNormal", normal, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {}, m_Sampler);
			m_InsertSurfelsKernel->UpdateImageView("gDepth", depth, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {}, m_Sampler);
		}

		uint32_t threadX = (screenWidth + 15) / 16;
		uint32_t threadY = (screenHeight + 15) / 16;
		
		//if (frameCounter <= 1)
		{
			m_InsertSurfelsKernel->Dispatch(threadX, threadY, 1, cmd);
		}
	}

	void VisualizeCellPass::SetSampler(VkSampler sampler)
	{
		m_Sampler = sampler;
	}

	void VisualizeCellPass::SetScreenResolution(uint32_t width, uint32_t height)
	{
		screenHeight = height;
		screenWidth = width;
	}

	void VisualizeCellPass::Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc)
	{
		CreateComputeKernel(m_VisualizeCellShaderKernel, "SurfelGI/visualizeGridCell.comp", GetRenderAPI());

		m_SurfelBufferAttachment = pass->AddBufferStorageInput(surfelBufferName, vkrg::BufferSlice::fullBuffer).value();
		m_SurfelASAttachment = pass->AddBufferStorageInput(surfelAccelerationStructureName, vkrg::BufferSlice::fullBuffer).value();
	
		vkrg::ImageSlice slice{};
		slice.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		slice.baseArrayLayer = 0;
		slice.baseMipLevel = 0;
		slice.layerCount = 1;
		slice.levelCount = 1;
		m_DepthAttachment = pass->AddImageColorInput("depth", slice, VK_IMAGE_VIEW_TYPE_2D).value();

		slice.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		m_BackBufferAttachment = pass->AddImageStorageOutput("backBuffer", slice, VK_IMAGE_VIEW_TYPE_2D).value();
		m_NormalAttachment = pass->AddImageColorInput("normal", slice, VK_IMAGE_VIEW_TYPE_2D).value();
		
	}

	void VisualizeCellPass::OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd)
	{
		if (ctx.CheckAttachmentDirtyFlag(m_SurfelASAttachment) || ctx.CheckAttachmentDirtyFlag(m_DepthAttachment)
			|| ctx.CheckAttachmentDirtyFlag(m_NormalAttachment))
		{
			vkrg::BufferView surfelASAttachment = ctx.GetBufferAttachment(m_SurfelASAttachment);
			m_VisualizeCellShaderKernel->UpdateBuffer("accelStruct", surfelASAttachment.buffer, surfelASAttachment.size,
				surfelASAttachment.offset);

			VkImageView depth = ctx.GetImageAttachment(m_DepthAttachment);
			m_VisualizeCellShaderKernel->UpdateImageView("gDepth", depth, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, m_Sampler);

			VkImageView normal = ctx.GetImageAttachment(m_NormalAttachment);
			m_VisualizeCellShaderKernel->UpdateImageView("gNormal", normal, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, m_Sampler);
		}
		
		VkImageView backBuffer = ctx.GetImageAttachment(m_BackBufferAttachment);
		m_VisualizeCellShaderKernel->UpdateImageView("backBuffer", backBuffer, VK_IMAGE_LAYOUT_GENERAL, {}, {});
		
		uint32_t threadX = (screenWidth + 15) / 16;
		uint32_t threadY = (screenHeight + 15) / 16;
		
		m_VisualizeCellShaderKernel->Dispatch(threadX, threadY, 1, cmd);
	}

	void BuildAccelerationStructurePass::Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc)
	{
		CreateComputeKernel(m_BuildASKernel, "SurfelGI/generateAccelerationStructure.comp", GetRenderAPI());

		m_SurfelBufferAttachment = pass->AddBufferStorageInput(surfelBufferName, vkrg::BufferSlice::fullBuffer).value();
		m_SurfelIndexBufferAttachment = pass->AddBufferStorageInput(surfelIndexBufferName, vkrg::BufferSlice::fullBuffer).value();
		m_SurfelASAttachment = pass->AddBufferStorageInput(surfelAccelerationStructureName, vkrg::BufferSlice::fullBuffer).value();
	}

	void BuildAccelerationStructurePass::OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd)
	{
		if (ctx.CheckAttachmentDirtyFlag(m_SurfelASAttachment))
		{
			vkrg::BufferView surfelASAttachment = ctx.GetBufferAttachment(m_SurfelASAttachment);
			m_BuildASKernel->UpdateBuffer("accelStruct", surfelASAttachment.buffer, surfelASAttachment.size,
				surfelASAttachment.offset);
		}

		//if (frameCounter <= 1)
		{
			m_BuildASKernel->Dispatch(m_SurfelGridCount, m_SurfelGridCount, m_SurfelGridCount, cmd);
		}
	}

	void SurfelPathTracingPass::AttachScene(ptr<RTScene> scene)
	{
		m_RTScene = scene;
	}

	void SurfelPathTracingPass::Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc)
	{
		m_SurfelAttachment = pass->AddBufferRTInput(surfelBufferName, vkrg::BufferSlice::fullBuffer).value();
		m_SurfelIndexAttachment = pass->AddBufferRTInput(surfelIndexBufferName, vkrg::BufferSlice::fullBuffer).value();

		ptr<ShaderManager> shaderManager = Singleton::GetInstance<AssetManager>()->GetShaderManager();
		ShaderID rtShader;

		{
			auto loadedShader = shaderManager->Load("SurfelGI/surfelPT.rt");
			KBS_ASSERT(loadedShader.has_value(), "fail to load shader {} for surfel gi renderer", "SurfelGI/surfelPT.rt");
			rtShader = loadedShader.value()->GetShaderID();
		}

		RenderAPI api = GetRenderAPI();
		{
			auto kernel = api.CreateRTKernel(rtShader);
			KBS_ASSERT(kernel.has_value(), "fail to create kernel for shader {} in surfel gi renderer", "SurfelGI/surfelPT.rt");
			m_Kernel = kernel.value();
		}

	}

	void SurfelPathTracingPass::OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd)
	{
		if (ctx.CheckAttachmentDirtyFlag(m_SurfelAttachment) || 
			ctx.CheckAttachmentDirtyFlag(m_SurfelIndexAttachment)) 
		{
			vkrg::BufferView surfelBuffer = ctx.GetBufferAttachment(m_SurfelAttachment);
			vkrg::BufferView surfelIndexBuffer = ctx.GetBufferAttachment(m_SurfelIndexAttachment);

			m_Kernel->UpdateBuffer("surfels", surfelBuffer.buffer, surfelBuffer.size, surfelBuffer.offset);
			m_Kernel->UpdateBuffer("surfelIdxBuffer", surfelIndexBuffer.buffer, surfelIndexBuffer.size, surfelIndexBuffer.offset);
		}
		
		m_Kernel->Dispatch(m_MaxSurfelBufferCount + 1, 1, 1, cmd);
		
	}

}

