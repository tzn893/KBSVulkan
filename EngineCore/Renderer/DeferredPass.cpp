#include "Renderer/DeferredPass.h"
#include "Asset/AssetManager.h"

using namespace vkrg;


void kbs::DeferredPass::Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc, ptr<gvk::Context> ctx)
{
	auto[colorAttachmentName, colorAttachmentView] = desc.RequireAttachment("color").value();
	auto[normalAttachmentName, normalAttachmentView] = desc.RequireAttachment("normal").value();
	auto[materialAttachmentName, materialAttachmentView] = desc.RequireAttachment("material").value();
	auto [depthStencilAttachmentName, depthStencilAttachmentView] = desc.RequireAttachment("depth").value();

	color = pass->AddImageColorOutput(colorAttachmentName.c_str(), colorAttachmentView).value();
	normal = pass->AddImageColorOutput(normalAttachmentName.c_str(), normalAttachmentView).value();
	material = pass->AddImageColorOutput(materialAttachmentName.c_str(), materialAttachmentView).value();
	depthStencil = pass->AddImageDepthOutput(depthStencilAttachmentName.c_str(), depthStencilAttachmentView).value();

	AssetManager* assetManager = Singleton::GetInstance<AssetManager>();
	ptr<ShaderManager> shaderManager = assetManager->GetShaderManager();
	
	{
		auto loadResult = shaderManager->Load("Deferred/mrt.frag");
		KBS_ASSERT(loadResult.has_value(), "fail to load mrt shader mrt.frag");

		m_MRTShader = std::dynamic_pointer_cast<GraphicsShader>(loadResult.value());
	}
}

void kbs::DeferredPass::OnSceneRender(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd, RenderCamera& camera, ptr<Scene> scene)
{
	RenderFilter filter;
	filter.flags = RenderPass_Opaque;
	filter.shaderFilter = [&](const ShaderID& id)
	{
		return id == m_MRTShader->GetShaderID();
	};
	
	RenderSceneByCamera(filter, cmd);
}

void kbs::DeferredPass::GetClearValue(uint32_t attachment, VkClearValue& value)
{
	if (attachment == depthStencil.idx)
	{
		value.depthStencil.depth = 1;
		value.depthStencil.stencil = 0;
	}
	else
	{
		value.color = { 0, 0, 0, 1 };
	}
}

void kbs::DeferredPass::GetAttachmentStoreLoadOperation(uint32_t attachment, VkAttachmentLoadOp& loadOp, VkAttachmentStoreOp& storeOp, VkAttachmentLoadOp& stencilLoadOp, VkAttachmentStoreOp& stencilStoreOp)
{
	loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
}


bool kbs::DeferredPass::OnValidationCheck(std::string& msg)
{
	if (depthStencil.type != RenderPassAttachment::ImageDepthOutput)
	{
		msg = "invalid depth stencil attachment for deferred render pass";
		return false;
	}

	if (!CheckAttachment(normal, "normal", msg))
	{
		return false;
	}
	if (!CheckAttachment(color, "color", msg))
	{
		return false;
	}
	if (!CheckAttachment(material, "material", msg))
	{
		return false;
	}

	return true;
}


ptr<kbs::GraphicsShader> kbs::DeferredPass::GetMRTShader()
{
	return m_MRTShader;
}

bool kbs::DeferredPass::CheckAttachment(RenderPassAttachment attachment, std::string attachment_name, std::string& msg)
{

	if (attachment.targetPass != m_TargetPass)
	{
		msg = std::string("invalid attachment ") + attachment_name + " deferred render pass interface must be binded to the same render pass as this attachment's";
		return false;
	}
	if (attachment.type != RenderPassAttachment::ImageColorOutput)
	{
		msg = std::string("invalid ") + attachment_name + " attachment for deferred render pass";
		return false;
	}
	if (m_TargetPass->GetAttachmentInfo(attachment).format != VK_FORMAT_R8G8B8A8_UNORM)
	{
		msg = std::string("invalid format for ") + attachment_name + " attachment : expected VK_FORMAT_R8G8B8A8_UNORM";
		return false;
	}

	return true;
}

namespace kbs
{
	struct DeferredPassLight
	{
		kbs::mat4    lightVP;
		kbs::vec3    emission;
		int     lightType;
		kbs::vec3    position;
		int     mainLightFlag;
		float   shadowBias;
		kbs::vec3    __padding;
	};

	struct DeferredPassUniform
	{
		kbs::mat4 invProjection;
		kbs::mat4 projection;
		kbs::mat4 invView;
		kbs::mat4 view;
		kbs::vec3 cameraPosition;
		int  lightCount;

		int screenWidth;
		int screenHeight;
	};
}



void kbs::DeferredShadingPass::AttachScene(ptr<Scene> scene, opt<Entity> mainLight, opt<kbs::mat4> mainLightMVP)
{
	bool mainLightFounded = false;
	RenderAPI api = GetRenderAPI();

	std::vector<DeferredPassLight> lights;
	scene->IterateAllEntitiesWith<LightComponent>(
		[&](Entity e)
		{
			LightComponent lc = e.GetComponent<LightComponent>();
	Transform lTrans(e);

	DeferredPassLight lightData;
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

	uint32_t deferredPassLightBufferSize = lights.size() * sizeof(DeferredPassLight);

	if (m_LightBuffer == nullptr || deferredPassLightBufferSize > m_LightBuffer->GetBuffer()->GetSize())
	{
		m_LightBuffer = api.CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, deferredPassLightBufferSize, GVK_HOST_WRITE_SEQUENTIAL);
		m_LightBuffer->Write(lights.data(), deferredPassLightBufferSize, 0);
		m_DeferredShadingKernel->UpdateBuffer("lights", m_LightBuffer->GetBuffer()->GetBuffer(),
			deferredPassLightBufferSize, 0);
	}
	
	RenderCamera renderCamera(scene->GetMainCamera());
	CameraUBO cameraUBO = renderCamera.GetCameraUBO();

	DeferredPassUniform uniformData;
	uniformData.cameraPosition = renderCamera.GetCameraTransform().GetPosition();
	uniformData.invProjection = cameraUBO.invProjection;
	uniformData.invView = cameraUBO.invView;
	uniformData.lightCount = lights.size();
	uniformData.view = cameraUBO.view;
	uniformData.projection = cameraUBO.projection;
	uniformData.screenWidth = m_Width;
	uniformData.screenHeight = m_Height;

	if (m_UniformBuffer == nullptr)
	{
		m_UniformBuffer = api.CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(uniformData), GVK_HOST_WRITE_SEQUENTIAL);
		m_DeferredShadingKernel->UpdateBuffer("uni", m_UniformBuffer);
	}

	m_UniformBuffer->Write(uniformData);
}

void kbs::DeferredShadingPass::ResizeScreen(uint32_t width, uint32_t height)
{
	m_Width = width;
	m_Height = height;
}

void kbs::DeferredShadingPass::Initialize(ptr<vkrg::RenderPass> pass, RendererAttachmentDescriptor& desc)
{
	auto [colorAttachmentName, colorAttachmentView] = desc.RequireAttachment("color").value();
	auto [normalAttachmentName, normalAttachmentView] = desc.RequireAttachment("normal").value();
	auto [materialAttachmentName, materialAttachmentView] = desc.RequireAttachment("material").value();
	auto [depthStencilAttachmentName, depthStencilAttachmentView] = desc.RequireAttachment("depth").value();
	
	bool hasShadow = false;
	std::string shadowAttachmentName;
	vkrg::ImageSlice shadowAttachmentView;
	if (auto var = desc.RequireAttachment("shadow");var.has_value())
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
	if (hasShadow)
	{
		m_ShadowMapAttachment = pass->AddImageColorInput(shadowAttachmentName.c_str(), shadowAttachmentView, VK_IMAGE_VIEW_TYPE_2D).value();
	}

	AssetManager* assetManager = Singleton::GetInstance<AssetManager>();
	ptr<ShaderManager> shaderManager = assetManager->GetShaderManager();
	ptr<ComputeShader> deferredShader;

	{
		auto loadResult = shaderManager->Load("Deferred/deferred.comp");
		KBS_ASSERT(loadResult.has_value(), "fail to load deferred shader deferred.comp");

		deferredShader = std::dynamic_pointer_cast<ComputeShader>(loadResult.value());
	}
	m_DeferredShadingKernel = GetRenderAPI().CreateComputeKernel(deferredShader->GetShaderID()).value();

	m_DefaultSampler = GetRenderAPI().CreateSampler(GvkSamplerCreateInfo()).value();
	m_NearestSampler = GetRenderAPI().CreateSampler(GvkSamplerCreateInfo(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST)).value();
}

void kbs::DeferredShadingPass::OnDispatch(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd)
{
	if (m_WhiteImageView == NULL)
	{
		AssetManager* assetManager = Singleton::GetInstance<AssetManager>();
		ptr<TextureManager> textureManager = assetManager->GetTextureManager();
		m_WhiteImageView = textureManager->GetTextureByID(textureManager->GetDefaultWhite()).value()->GetMainView();
	}

	if (ctx.CheckAttachmentDirtyFlag(m_GBufferColorAttachment) || ctx.CheckAttachmentDirtyFlag(m_GBufferDepthAttachment)
		|| ctx.CheckAttachmentDirtyFlag(m_GBufferMaterialAttachment) || ctx.CheckAttachmentDirtyFlag(m_GBufferNormalAttachment))
	{
		VkImageView colorView = ctx.GetImageAttachment(m_GBufferColorAttachment);
		VkImageView depthView = ctx.GetImageAttachment(m_GBufferDepthAttachment);
		VkImageView materialView = ctx.GetImageAttachment(m_GBufferMaterialAttachment);
		VkImageView normalView = ctx.GetImageAttachment(m_GBufferNormalAttachment);

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
	}

	VkImageView colorInputView = ctx.GetImageAttachment(m_ColorImageAttachment);
	m_DeferredShadingKernel->UpdateImageView("colorImage", colorInputView, VK_IMAGE_LAYOUT_GENERAL, {}, {});

	m_DeferredShadingKernel->Dispatch((m_Width + 15) / 16, (m_Height + 15) / 16, 1, cmd);
}
