#include "Material.h"
#include "Core/Log.h"

namespace kbs
{

    kbs::Material::Material(const UUID& id, const std::string& name, ptr<GraphicsShader> shader, ptr<RenderBuffer> buffer)
        :m_Shader(shader), m_MaterialBuffer(buffer), m_Name(name), m_ID(id)
    {
        if (buffer != nullptr)
        {
            KBS_ASSERT(m_MaterialBuffer->GetBuffer()->GetSize() == shader->GetShaderReflection().GetVariableBufferSize(),
                "render buffer passed to material must have the same size as variable buffer in shader");
        }
        else
        {
            KBS_ASSERT(shader->GetShaderReflection().GetVariableBufferSize() == 0, "render buffer passed to material must have the same size as variable buffer in shader");
        }
    }

    void kbs::Material::SetInt(const std::string& name, int val)
    {
        auto var = m_Shader->GetShaderReflection().GetVariable(name);

        if (var.has_value() && var.value().type == ShaderReflection::VariableType::Int)
        {
            m_MaterialBuffer->GetBuffer()->Write(&val, var.value().offset, sizeof(int));
        }
    }

    void kbs::Material::SetFloat(const std::string& name, float val)
    {
        auto var = m_Shader->GetShaderReflection().GetVariable(name);

        if (var.has_value() && var.value().type == ShaderReflection::VariableType::Float)
        {
            m_MaterialBuffer->GetBuffer()->Write(&val, var.value().offset, sizeof(float));
        }
    }

    void kbs::Material::SetVec2(const std::string& name, vec2 val)
    {
        auto var = m_Shader->GetShaderReflection().GetVariable(name);

        if (var.has_value() && var.value().type == ShaderReflection::VariableType::Float2)
        {
            m_MaterialBuffer->GetBuffer()->Write(&val, var.value().offset, sizeof(vec2));
        }
    }

    void kbs::Material::SetVec3(const std::string& name, vec3 val)
    {
        auto var = m_Shader->GetShaderReflection().GetVariable(name);

        if (var.has_value() && var.value().type == ShaderReflection::VariableType::Float3)
        {
            m_MaterialBuffer->GetBuffer()->Write(&val, var.value().offset, sizeof(vec3));
        }
    }

    void kbs::Material::SetVec4(const std::string& name, vec4 val)
    {
        auto var = m_Shader->GetShaderReflection().GetVariable(name);

        if (var.has_value() && var.value().type == ShaderReflection::VariableType::Float4)
        {
            m_MaterialBuffer->GetBuffer()->Write(&val, var.value().offset, sizeof(vec4));
        }
    }

    void kbs::Material::SetMat4(const std::string& name, mat4 val)
    {
        auto var = m_Shader->GetShaderReflection().GetVariable(name);

        if (var.has_value() && var.value().type == ShaderReflection::VariableType::Mat4)
        {
            m_MaterialBuffer->GetBuffer()->Write(&val, var.value().offset, sizeof(mat4));
        }
    }

    void kbs::Material::SetBuffer(const std::string& name, ptr<RenderBuffer> buffer)
    {
        auto var = m_Shader->GetShaderReflection().GetBuffer(name);

        if (var.has_value() && var.value().set == (uint32_t)ShaderSetUsage::perMaterial)
        {
            m_BufferBindings.push_back(MaterialBufferBinding{ var.value(), buffer });
        }
    }

    void kbs::Material::SetTexture(const std::string& name, ptr<Texture> texture)
    {
        auto var = m_Shader->GetShaderReflection().GetTexture(name);

        if (var.has_value() && var.value().set == (uint32_t)ShaderSetUsage::perMaterial)
        {
            m_TextureBindings.push_back(MaterialTextureBinding{ var.value(), texture });
        }
    }

    kbs::ptr<kbs::GraphicsShader> kbs::Material::GetShader()
    {
        return m_Shader;
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
                write.BufferWrite(set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, m_MaterialBuffer->GetBuffer()->GetBuffer(), 0, m_Shader->GetShaderReflection().GetVariableBufferSize());
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
                // currently only combined image sampler is supported
                write.ImageWrite(set, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, tex.info.set, tex.tex->GetSampler(),
                    tex.tex->GetMainView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
        return m_Shader->GetRenderPassFlags();
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
        auto mat = std::make_shared<Material>(UUID::GenerateUncollidedID(m_MaterialIDTable), name, shader, buffer);

        m_MaterialIDTable[mat->GetID()] = mat;
        m_Materials.push_back(mat);

        return mat->GetID();
    }

    MaterialID MaterialManager::CreateMaterial(ptr<GraphicsShader> shader, RenderAPI& api, const std::string& name)
    {
        uint32_t materialBufferSize = shader->GetShaderReflection().GetVariableBufferSize();
        ptr<RenderBuffer> buffer = api.CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, materialBufferSize, GVK_HOST_WRITE_SEQUENTIAL);

        return CreateMaterial(shader, buffer, name);
    }


}
