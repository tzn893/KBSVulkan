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
		auto loadResult = shaderManager->Load("mrt.frag");
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

