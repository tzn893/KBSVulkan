#include "Material.h"
#include "Core/Log.h"
#include "Asset/AssetManager.h"

namespace kbs
{

    kbs::Material::Material(const UUID& id, const std::string& name, ShaderID shader, ptr<RenderBuffer> buffer)
        :m_ShaderID(shader), m_MaterialBuffer(buffer), m_Name(name), m_ID(id)
    {
        if (buffer != nullptr)
        {
            KBS_ASSERT(m_MaterialBuffer->GetBuffer()->GetSize() == GetShaderByID()->GetShaderReflection().GetVariableBufferSize(),
                "render buffer passed to material must have the same size as variable buffer in shader");
        }
        else
        {
            KBS_ASSERT(GetShaderByID()->GetShaderReflection().GetVariableBufferSize() == 0, "render buffer passed to material must have the same size as variable buffer in shader");
        }
    }

    void kbs::Material::SetInt(const std::string& name, int val)
    {
        auto var = GetShaderByID()->GetShaderReflection().GetVariable(name);

        if (var.has_value() && var.value().type == ShaderReflection::VariableType::Int)
        {
            m_MaterialBuffer->GetBuffer()->Write(&val, var.value().offset, sizeof(int));
        }
    }

    void kbs::Material::SetFloat(const std::string& name, float val)
    {
        auto var = GetShaderByID()->GetShaderReflection().GetVariable(name);

        if (var.has_value() && var.value().type == ShaderReflection::VariableType::Float)
        {
            m_MaterialBuffer->GetBuffer()->Write(&val, var.value().offset, sizeof(float));
        }
    }

    void kbs::Material::SetVec2(const std::string& name, vec2 val)
    {
        auto var = GetShaderByID()->GetShaderReflection().GetVariable(name);

        if (var.has_value() && var.value().type == ShaderReflection::VariableType::Float2)
        {
            m_MaterialBuffer->GetBuffer()->Write(&val, var.value().offset, sizeof(vec2));
        }
    }

    void kbs::Material::SetVec3(const std::string& name, vec3 val)
    {
        auto var = GetShaderByID()->GetShaderReflection().GetVariable(name);

        if (var.has_value() && var.value().type == ShaderReflection::VariableType::Float3)
        {
            m_MaterialBuffer->GetBuffer()->Write(&val, var.value().offset, sizeof(vec3));
        }
    }

    void kbs::Material::SetVec4(const std::string& name, vec4 val)
    {
        auto var = GetShaderByID()->GetShaderReflection().GetVariable(name);

        if (var.has_value() && var.value().type == ShaderReflection::VariableType::Float4)
        {
            m_MaterialBuffer->GetBuffer()->Write(&val, var.value().offset, sizeof(vec4));
        }
    }

    void kbs::Material::SetMat4(const std::string& name, mat4 val)
    {
        auto var = GetShaderByID()->GetShaderReflection().GetVariable(name);

        if (var.has_value() && var.value().type == ShaderReflection::VariableType::Mat4)
        {
            m_MaterialBuffer->GetBuffer()->Write(&val, var.value().offset, sizeof(mat4));
        }
    }

    void kbs::Material::SetBuffer(const std::string& name, ptr<RenderBuffer> buffer)
    {
        auto var = GetShaderByID()->GetShaderReflection().GetBuffer(name);

        if (var.has_value() && var.value().set == (uint32_t)ShaderSetUsage::perMaterial)
        {
            m_BufferBindings.push_back(MaterialBufferBinding{ var.value(), buffer });
        }
    }

    void kbs::Material::SetTexture(const std::string& name, ptr<Texture> texture)
    {
        auto var = GetShaderByID()->GetShaderReflection().GetTexture(name);

        if (var.has_value() && var.value().set == (uint32_t)ShaderSetUsage::perMaterial)
        {
            m_TextureBindings.push_back(MaterialTextureBinding{ var.value(), texture });
        }
    }

    kbs::ptr<kbs::GraphicsShader> kbs::Material::GetShader()
    {
        return GetShaderByID();
    }

    kbs::ptr<kbs::RenderBuffer> kbs::Material::GetBuffer()
    {
        return m_MaterialBuffer;
    }


    void kbs::Material::UpdateDescriptorSet(ptr<gvk::Context> ctx, ptr<gvk::DescriptorSet> set)
    {
        if (set->GetSetIndex() == (uint32_t)ShaderSetUsage::perMaterial)
        {
            GvkDescriptorSetWrite write;

            if (m_MaterialBuffer != nullptr)
            {
                write.BufferWrite(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, m_MaterialBuffer->GetBuffer()->GetBuffer(), 0,GetShaderByID()->GetShaderReflection().GetVariableBufferSize());
            }

            for (auto& buf : m_BufferBindings)
            {

                VkDescriptorType type;
                switch (buf.info.type)
                {
                case ShaderReflection::BufferType::Storage:
                    type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    break;
                case ShaderReflection::BufferType::Uniform:
                default:
                    type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    break;
                }
                write.BufferWrite(set, type, buf.info.binding, buf.buf->GetBuffer()->GetBuffer(), 0, buf.buf->GetBuffer()->GetSize());
            }

            for (auto& tex : m_TextureBindings)
            {
                VkDescriptorType type;
                switch (tex.info.type)
                {
                case ShaderReflection::TextureType::CombinedImageSampler:
                    type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    break;
                case ShaderReflection::TextureType::StorageImage:
                    type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    break;
                }
                write.ImageWrite(set, type, tex.info.binding, tex.tex->GetSampler(), tex.tex->GetMainView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            write.Emit(ctx->GetDevice());
        }
    }

    std::string kbs::Material::GetName()
    {
        return m_Name;
    }

    kbs::MaterialID kbs::Material::GetID()
    {
        return m_ID;
    }

    RenderPassFlags Material::GetRenderPassFlags()
    {
        return GetShaderByID()->GetRenderPassFlags();
    }

    ptr<GraphicsShader> Material::GetShaderByID()
    {
        ptr<ShaderManager> shaderManager = Singleton::GetInstance<AssetManager>()->GetShaderManager();
        auto shader = shaderManager->Get(m_ShaderID);
        KBS_ASSERT(shader.has_value(), "shader id passed to material must be valid");
        KBS_ASSERT(shader.value()->IsGraphicsShader(), "shader passed to material must be a graphics shader");

        return std::dynamic_pointer_cast<GraphicsShader>(shader.value());
    }

    View<ptr<Material>> kbs::MaterialManager::GetMaterials()
    {
        return View(m_Materials);
    }

    opt<ptr<Material>> kbs::MaterialManager::GetMaterialByID(const MaterialID& id)
    {
        if (m_MaterialIDTable.count(id))
        {
            return m_MaterialIDTable[id];
        }
        return std::nullopt;
    }

    kbs::MaterialID kbs::MaterialManager::CreateMaterial(ptr<GraphicsShader> shader, ptr<RenderBuffer> buffer, const std::string& name)
    {
        auto mat = std::make_shared<Material>(UUID::GenerateUncollidedID(m_MaterialIDTable), name, shader->GetShaderID(), buffer);

        m_MaterialIDTable[mat->GetID()] = mat;
        m_Materials.push_back(mat);

        return mat->GetID();
    }

    MaterialID MaterialManager::CreateMaterial(ptr<GraphicsShader> shader, RenderAPI& api, const std::string& name)
    {
        uint32_t materialBufferSize = shader->GetShaderReflection().GetVariableBufferSize();
        ptr<RenderBuffer> buffer = nullptr;
        if (materialBufferSize != 0)
        {
            buffer = api.CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, materialBufferSize, GVK_HOST_WRITE_SEQUENTIAL);
        }

        return CreateMaterial(shader, buffer, name);
    }

    opt<ptr<RTMaterial>> MaterialManager::GetRTMaterialByID(const RTMaterialID& id)
    {
        if (m_RTMaterialIDTable.count(id))
        {
            return m_RTMaterialIDTable[id];
        }
        return std::nullopt;
    }

    RTMaterialID MaterialManager::CreateRTMaterial(const std::string& name)
    {
        RTMaterialID matID = UUID::GenerateUncollidedID(m_RTMaterialIDTable);
        ptr<RTMaterial> rtMaterial = std::make_shared<RTMaterial>(matID, name);

        m_RTMaterialIDTable[matID] = rtMaterial;
        return matID;
    }


    RTMaterial::RTMaterial(RTMaterialID matID, const std::string& name)
        :m_RTMaterialID(matID),m_Name(name)
    {

    }

    void RTMaterial::SetDiffuseTexture(TextureID diffuseTexture)
    {
        m_DiffuseTexture = diffuseTexture;
    }

    TextureID RTMaterial::GetDiffuseTexture()
    {
        return m_DiffuseTexture;
    }

    void RTMaterial::SetMetallicTexture(TextureID metallicTexture)
    {
        m_MetallicTexture = metallicTexture;
    }

    TextureID RTMaterial::GetMetallicTexture()
    {
        return m_MetallicTexture;
    }

    void RTMaterial::SetEmissiveTexture(TextureID emissiveTexture)
    {
        m_EmissiveTexture = emissiveTexture;
    }

    TextureID RTMaterial::GetEmissiveTexture()
    {
        return m_EmissiveTexture;
    }

	void RTMaterial::SetNormalTexture(TextureID normalTexture)
	{
        m_NormalTexture = normalTexture;
	}

	kbs::TextureID RTMaterial::GetNormalTexture()
	{
        return m_NormalTexture;
	}

	kbs::PBRMaterialParameter& RTMaterial::GetMaterialParameter()
	{
        return m_PBRParameter;
	}

	std::string RTMaterial::GetName()
    {
        return m_Name;
    }

}
