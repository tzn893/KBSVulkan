#include "TNNAORenderer.h"
#include "Core/Event.h"
#include "Asset/AssetManager.h"
#include "Core/FileSystem.h"


namespace kbs
{

	struct TNNAODeferredPassLight
	{
		kbs::mat4    lightVP;
		kbs::vec3    emission;
		int     lightType;
		kbs::vec3    position;
		int     mainLightFlag;
		float   shadowBias;
		kbs::vec3    __padding;
	};

	struct TNNAODeferredPassUniform
	{
		kbs::mat4 invProjection;
		kbs::mat4 projection;
		kbs::mat4 invView;
		kbs::mat4 view;
		kbs::vec3 cameraPosition;
		int  lightCount;

		int screenWidth;
		int screenHeight;
		float radius;
	};

	void TNNAODeferredShadingPass::AttachScene(ptr<Scene> scene, opt<Entity> mainLight, opt<kbs::mat4> mainLightMVP)
	{
		bool mainLightFounded = false;
		RenderAPI api = GetRenderAPI();

		std::vector<TNNAODeferredPassLight> lights;
		scene->IterateAllEntitiesWith<LightComponent>(
			[&](Entity e)
			{
				LightComponent lc = e.GetComponent<LightComponent>();
		Transform lTrans(e);

		TNNAODeferredPassLight lightData;
		lightData.emission = lc.intensity;
		lightData.lightType = lc.type;
		lightData.shadowBias = 1e-4;
		lightData.mainLightFlag = false;

		if (lc.type == LightComponent::Directional)
		{
			lightData.position = -lTrans.GetFront();

			if (mainLight.has_value())
			{
				if (e.GetUUID() == mainLight.value().GetUUID())
				{
					lightData.mainLightFlag = true;
					ObjectUBO objectUBO = lTrans.GetObjectUBO();
					lightData.lightVP = mainLightMVP.value();
				}
			}
		}

		lights.push_back(lightData);
			}
		);

		uint32_t deferredPassLightBufferSize = lights.size() * sizeof(TNNAODeferredPassLight);

		if (m_LightBuffer == nullptr || deferredPassLightBufferSize > m_LightBuffer->GetBuffer()->GetSize())
		{
			m_LightBuffer = api.CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, deferredPassLightBufferSize, GVK_HOST_WRITE_SEQUENTIAL);
			m_LightBuffer->Write(lights.data(), deferredPassLightBufferSize, 0);
			m_DeferredShadingKernel->UpdateBuffer("lights", m_LightBuffer->GetBuffer()->GetBuffer(),
				deferredPassLightBufferSize, 0);
		}

		RenderCamera renderCamera(scene->GetMainCamera());
		CameraUBO cameraUBO = renderCamera.GetCameraUBO();

		TNNAODeferredPassUniform uniformData;
		uniformData.cameraPosition = renderCamera.GetCameraTransform().GetPosition();
		uniformData.invProjection = cameraUBO.invProjection;
		uniformData.invView = cameraUBO.invView;
		uniformData.lightCount = lights.size();
		uniformData.view = cameraUBO.view;
		uniformData.projection = cameraUBO.projection;
		uniformData.screenWidth = m_Width;
		uniformData.screenHeight = m_Height;
		uniformData.radius = 1.0f;

		if (m_UniformBuffer == nullptr)
		{
			m_UniformBuffer = api.CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(uniformData), GVK_HOST_WRITE_SEQUENTIAL);
			m_DeferredShadingKernel->UpdateBuffer("uni", m_UniformBuffer);
		}

		m_UniformBuffer->Write(uniformData);
	}

	void TNNAODeferredShadingPass::ResizeScreen(uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;
	}

	ptr<kbs::RenderBuffer> TNNAODeferredShadingPass::GetUniformBuffer()
	{
		return m_UniformBuffer;
	}

	void TNNAODeferredShadingPass::Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc)
	{
		auto [colorAttachmentName, colorAttachmentView] = desc.RequireAttachment("color").value();
		auto [normalAttachmentName, normalAttachmentView] = desc.RequireAttachment("normal").value();
		auto [materialAttachmentName, materialAttachmentView] = desc.RequireAttachment("material").value();
		auto [depthStencilAttachmentName, depthStencilAttachmentView] = desc.RequireAttachment("depth").value();
		auto [aoAttachmentName, aoAttachmentSlice] = desc.RequireAttachment("ao").value();

		bool hasShadow = false;
		std::string shadowAttachmentName;
		vkrg::ImageSlice shadowAttachmentView;
		if (auto var = desc.RequireAttachment("shadow"); var.has_value())
		{
			shadowAttachmentName = std::get<0>(var.value());
			shadowAttachmentView = std::get<1>(var.value());
			hasShadow = true;
		}

		auto [colorImageAttachmentName, colorImageAttachmentView] = desc.RequireAttachment("colorOut").value();

		m_GBufferColorAttachment = pass->AddImageColorInput(colorAttachmentName.c_str(), colorAttachmentView, VK_IMAGE_VIEW_TYPE_2D).value();
		m_GBufferNormalAttachment = pass->AddImageColorInput(normalAttachmentName.c_str(), normalAttachmentView, VK_IMAGE_VIEW_TYPE_2D).value();
		m_GBufferMaterialAttachment = pass->AddImageColorInput(materialAttachmentName.c_str(), materialAttachmentView, VK_IMAGE_VIEW_TYPE_2D).value();
		m_GBufferDepthAttachment = pass->AddImageColorInput(depthStencilAttachmentName.c_str(), depthStencilAttachmentView, VK_IMAGE_VIEW_TYPE_2D).value();

		m_ColorImageAttachment = pass->AddImageStorageOutput(colorImageAttachmentName.c_str(), colorImageAttachmentView, VK_IMAGE_VIEW_TYPE_2D).value();
		m_AOAttachment = pass->AddImageColorInput(aoAttachmentName.c_str(), aoAttachmentSlice, VK_IMAGE_VIEW_TYPE_2D).value();
		if (hasShadow)
		{
			m_ShadowMapAttachment = pass->AddImageColorInput(shadowAttachmentName.c_str(), shadowAttachmentView, VK_IMAGE_VIEW_TYPE_2D).value();
		}

		AssetManager* assetManager = Singleton::GetInstance<AssetManager>();
		ptr<ShaderManager> shaderManager = assetManager->GetShaderManager();
		ptr<ComputeShader> deferredShader;

		{
			auto loadResult = shaderManager->Load("TNNAODeferred.comp");
			KBS_ASSERT(loadResult.has_value(), "fail to load deferred shader TNNAODeferred.comp");

			deferredShader = std::dynamic_pointer_cast<ComputeShader>(loadResult.value());
		}
		m_DeferredShadingKernel = GetRenderAPI().CreateComputeKernel(deferredShader->GetShaderID()).value();

		m_DefaultSampler = GetRenderAPI().CreateSampler(GvkSamplerCreateInfo()).value();
		m_NearestSampler = GetRenderAPI().CreateSampler(GvkSamplerCreateInfo(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST)).value();
	}

	void TNNAODeferredShadingPass::OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd)
	{
		if (m_WhiteImageView == NULL)
		{
			AssetManager* assetManager = Singleton::GetInstance<AssetManager>();
			ptr<TextureManager> textureManager = assetManager->GetTextureManager();
			m_WhiteImageView = textureManager->GetTextureByID(textureManager->GetDefaultWhite()).value()->GetMainView();
		}

		if (ctx.CheckAttachmentDirtyFlag(m_GBufferColorAttachment) || ctx.CheckAttachmentDirtyFlag(m_GBufferDepthAttachment)
			|| ctx.CheckAttachmentDirtyFlag(m_GBufferMaterialAttachment) || ctx.CheckAttachmentDirtyFlag(m_GBufferNormalAttachment)
			|| ctx.CheckAttachmentDirtyFlag(m_AOAttachment))
		{
			VkImageView colorView = ctx.GetImageAttachment(m_GBufferColorAttachment);
			VkImageView depthView = ctx.GetImageAttachment(m_GBufferDepthAttachment);
			VkImageView materialView = ctx.GetImageAttachment(m_GBufferMaterialAttachment);
			VkImageView normalView = ctx.GetImageAttachment(m_GBufferNormalAttachment);
			VkImageView aoView = ctx.GetImageAttachment(m_AOAttachment);

			if (m_ShadowMapAttachment.has_value())
			{
				VkImageView shadowView = ctx.GetImageAttachment(m_ShadowMapAttachment.value());
				m_DeferredShadingKernel->UpdateImageView("shadowMap", shadowView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {}, m_DefaultSampler);
			}
			else
			{
				m_DeferredShadingKernel->UpdateImageView("shadowMap", m_WhiteImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {}, m_DefaultSampler);
			}

			m_DeferredShadingKernel->UpdateImageView("gbufferDepth", depthView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {}, m_DefaultSampler);
			m_DeferredShadingKernel->UpdateImageView("gbufferMaterial", materialView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {}, m_NearestSampler);
			m_DeferredShadingKernel->UpdateImageView("gbufferNormal", normalView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {}, m_NearestSampler);
			m_DeferredShadingKernel->UpdateImageView("gbufferColor", colorView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {}, m_NearestSampler);
			m_DeferredShadingKernel->UpdateImageView("aoImage", aoView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {}, m_DefaultSampler);
		}

		VkImageView colorInputView = ctx.GetImageAttachment(m_ColorImageAttachment);
		m_DeferredShadingKernel->UpdateImageView("colorImage", colorInputView, VK_IMAGE_LAYOUT_GENERAL, {}, {});

		m_DeferredShadingKernel->Dispatch((m_Width + 15) / 16, (m_Height + 15) / 16, 1, cmd);
	}


	void TNNAOPass::ResizeScreen(uint32_t width, uint32_t height)
	{
		m_Width = width;
		m_Height = height;
	}

	void TNNAOPass::SetUniformBuffer(ptr<RenderBuffer> buffer)
	{
		m_TNNAOKernel->UpdateBuffer("uni", buffer);
	}

	void TNNAOPass::Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc)
	{
		auto [normalAttachmentName, normalAttachmentView] = desc.RequireAttachment("normal").value();
		auto [depthStencilAttachmentName, depthStencilAttachmentView] = desc.RequireAttachment("depth").value();

		bool hasShadow = false;
		std::string shadowAttachmentName;
		vkrg::ImageSlice shadowAttachmentView;
		if (auto var = desc.RequireAttachment("shadow"); var.has_value())
		{
			shadowAttachmentName = std::get<0>(var.value());
			shadowAttachmentView = std::get<1>(var.value());
			hasShadow = true;
		}

		auto [colorImageAttachmentName, colorImageAttachmentView] = desc.RequireAttachment("ao").value();

		m_GBufferNormalAttachment = pass->AddImageColorInput(normalAttachmentName.c_str(), normalAttachmentView, VK_IMAGE_VIEW_TYPE_2D).value();
		m_GBufferDepthAttachment = pass->AddImageColorInput(depthStencilAttachmentName.c_str(), depthStencilAttachmentView, VK_IMAGE_VIEW_TYPE_2D).value();

		m_ColorImageAttachment = pass->AddImageStorageOutput(colorImageAttachmentName.c_str(), colorImageAttachmentView, VK_IMAGE_VIEW_TYPE_2D).value();

		AssetManager* assetManager = Singleton::GetInstance<AssetManager>();
		ptr<ShaderManager> shaderManager = assetManager->GetShaderManager();


		Singleton::GetInstance<FileSystem<TextureManager>>()->AddSearchPath(TNNAO_ROOT_DIRECTORY);

		ptr<TextureManager> textureManager = assetManager->GetTextureManager();
		auto F0_ID = textureManager->Load("nnao_f0.tga", GetRenderAPI(), {}, GvkSamplerCreateInfo(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST)).value();
		auto F1_ID = textureManager->Load("nnao_f1.tga", GetRenderAPI(), {}, GvkSamplerCreateInfo(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST)).value();
		auto F2_ID = textureManager->Load("nnao_f2.tga", GetRenderAPI(), {}, GvkSamplerCreateInfo(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST)).value();
		auto F3_ID = textureManager->Load("nnao_f3.tga", GetRenderAPI(), {}, GvkSamplerCreateInfo(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST)).value();

		{
			ptr<ComputeShader> nnaoShader;

			auto loadResult = shaderManager->Load("nnao.comp");
			KBS_ASSERT(loadResult.has_value(), "fail to load deferred shader deferred.comp");

			nnaoShader = std::dynamic_pointer_cast<ComputeShader>(loadResult.value());

			m_TNNAOKernel = GetRenderAPI().CreateComputeKernel(nnaoShader->GetShaderID()).value();
		}

		m_DefaultSampler = GetRenderAPI().CreateSampler(GvkSamplerCreateInfo()).value();
		m_NearestSampler = GetRenderAPI().CreateSampler(GvkSamplerCreateInfo(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST)).value();

		{
			ptr<Texture> f0 = textureManager->GetTextureByID(F0_ID).value();
			m_TNNAOKernel->UpdateTexture("F0", f0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		{
			ptr<Texture> f1 = textureManager->GetTextureByID(F1_ID).value();
			m_TNNAOKernel->UpdateTexture("F1", f1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		{
			ptr<Texture> f2 = textureManager->GetTextureByID(F2_ID).value();
			m_TNNAOKernel->UpdateTexture("F2", f2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		{
			ptr<Texture> f3 = textureManager->GetTextureByID(F3_ID).value();
			m_TNNAOKernel->UpdateTexture("F3", f3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

	}

	void TNNAOPass::OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd)
	{
		if (ctx.CheckAttachmentDirtyFlag(m_GBufferDepthAttachment) ||
			ctx.CheckAttachmentDirtyFlag(m_GBufferNormalAttachment) ||
			ctx.CheckAttachmentDirtyFlag(m_ColorImageAttachment))
		{
			VkImageView depthView = ctx.GetImageAttachment(m_GBufferDepthAttachment);
			VkImageView normalView = ctx.GetImageAttachment(m_GBufferNormalAttachment);
			VkImageView colorInputView = ctx.GetImageAttachment(m_ColorImageAttachment);

			m_TNNAOKernel->UpdateImageView("colorImage", colorInputView, VK_IMAGE_LAYOUT_GENERAL, {}, {});
			m_TNNAOKernel->UpdateImageView("gbufferDepth", depthView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {}, m_DefaultSampler);
			m_TNNAOKernel->UpdateImageView("gbufferNormal", normalView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, {}, m_NearestSampler);
		}


		m_TNNAOKernel->Dispatch((m_Width + 15) / 16, (m_Height + 15) / 16, 1, cmd);
	}


	vkrg::RenderPassHandle kbs::TNNAORenderer::GetMaterialTargetRenderPass(RenderPassFlags flag)
	{
		if (kbs_contains_flags(flag, RenderPass_Shadow))
		{
			return m_ShadowPassHandle;
		}
		return m_DeferredPassHandle;
	}


	ptr<kbs::DeferredPass> TNNAORenderer::GetDeferredPass()
	{
		return m_DeferredPass;
	}

	bool TNNAORenderer::InitRenderGraph(ptr<kbs::Window> window, ptr<vkrg::RenderGraph> renderGraph)
	{
		vkrg::ResourceInfo info;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		renderGraph->AddGraphResource("normal", info, false);
		renderGraph->AddGraphResource("color", info, false);
		renderGraph->AddGraphResource("material", info, false);

		// info.format = VK_FORMAT_R8_UNORM;
		renderGraph->AddGraphResource("ao", info, false);

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
			attachmentDesc.AddAttachment("ao", "ao", slice);

			slice.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			attachmentDesc.AddAttachment("depth", "depth", slice);
			slice.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			attachmentDesc.AddAttachment("shadow", "shadow", slice);

			slice.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			attachmentDesc.AddAttachment("colorOut", "backBuffer", slice);

			m_DeferredShadingPass = CreateComputePass<TNNAODeferredShadingPass>(attachmentDesc, "deferredShadingPass");
			m_DeferredShadingPassHandle = m_DeferredShadingPass->GetRenderPassHandle();
		}


		{
			vkrg::ImageSlice slice;
			slice.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			slice.baseArrayLayer = 0;
			slice.baseMipLevel = 0;
			slice.layerCount = 1;
			slice.levelCount = 1;

			RendererAttachmentDescriptor attachmentDesc;
			attachmentDesc.AddAttachment("depth", "depth", slice);

			slice.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			attachmentDesc.AddAttachment("normal", "normal", slice);
			attachmentDesc.AddAttachment("ao", "ao", slice);

			m_TNNAOPass = CreateComputePass<TNNAOPass>(attachmentDesc, "nnao");

			m_TNNAOPassHandle = m_TNNAOPass->GetRenderPassHandle();
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
				m_TNNAOPass->ResizeScreen(e.GetWidth(), e.GetHeight());
			}
		);

		return true;
	}

	void TNNAORenderer::OnSceneRender(ptr<Scene> scene)
	{
		Entity mainCamera = scene->GetMainCamera();


		m_DeferredPass->SetTargetCamera(mainCamera);
		m_DeferredPass->SetTargetScene(scene);

		m_ShadowPass->SetTargetScene(scene);
		m_ShadowPass->SetTargetCamera(mainCamera);

		m_DeferredShadingPass->AttachScene(scene, m_ShadowPass->GetShadowCaster(), m_ShadowPass->GetLightVP());

		if (GetCurrentFrameIdx() == 0)
		{
			m_TNNAOPass->SetUniformBuffer(m_DeferredShadingPass->GetUniformBuffer());
		}
	}
}