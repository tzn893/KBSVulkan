#pragma once
#include "Shader.h"
#include "Math/math.h"
#include "Renderer/RenderResource.h"
#include "Scene/UUID.h"
#include "Renderer/RenderAPI.h"
#include "Asset/TextureManager.h"

namespace kbs
{
	using MaterialID = UUID;
	using RTMaterialID = UUID;

	class Material
	{
	public:
		Material(const UUID& uuid,const std::string& name,ShaderID shader, ptr<RenderBuffer> buffer);
		
		void SetInt    (const std::string& name, int val);
		void SetFloat(const std::string& name, float val);
		void SetVec2(const std::string& name, vec2 val);
		void SetVec3(const std::string& name, vec3 val);
		void SetVec4(const std::string& name, vec4 val);

		void SetMat4(const std::string& name, mat4 val);

		void SetBuffer(const std::string& name, ptr<RenderBuffer> buffer);
		void SetTexture(const std::string& name, ptr<Texture> texture);
		
		ptr<GraphicsShader>	   GetShader();
		ptr<RenderBuffer>	   GetBuffer();

		void				   UpdateDescriptorSet(ptr<gvk::Context> ctx, ptr<gvk::DescriptorSet> set);
		std::string			   GetName();
		MaterialID			   GetID();

		RenderPassFlags		   GetRenderPassFlags();

	private:
		ptr<GraphicsShader> GetShaderByID();


		std::string				m_Name;
		UUID					m_ID;
		//ptr<GraphicsShader>		m_Shader;
		ShaderID				m_ShaderID;
		ptr<RenderBuffer>		m_MaterialBuffer;

		struct MaterialBufferBinding
		{
			ShaderReflection::BufferInfo info;
			ptr<RenderBuffer>		     buf;
		};
		struct MaterialTextureBinding
		{
			ShaderReflection::TextureInfo info;
			ptr<Texture>				  tex;
		};
		std::vector<MaterialBufferBinding> m_BufferBindings;
		std::vector<MaterialTextureBinding> m_TextureBindings;
	};

	struct PBRMaterialParameter
	{
		PBRMaterialParameter()
		{
			albedo = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);
			extinction = glm::vec4(1.0f, 1.0f, 1.0f, 0.f);
			emission = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
			metallic = 0.0f;
			roughness = 0.5f;
			ior = 1.45f;
			atDistance = 1.f;
		}
		
		vec4 albedo{};
		vec4 emission{};
		vec4 extinction{};

		glm::float32_t metallic{};
		glm::float32_t roughness{};
		glm::float32_t subsurface{};
		glm::float32_t specularTint{};

		glm::float32_t sheen{};
		glm::float32_t sheenTint{};
		glm::float32_t clearcoat{};
		glm::float32_t clearcoatGloss{};

		glm::float32_t transmission{};
		glm::float32_t ior{};
		alignas(8) glm::float32_t atDistance{};
	};

	class RTMaterial
	{
	public:
		RTMaterial(RTMaterialID matID, const std::string& name);
		
		void			SetDiffuseTexture(TextureID diffuseTexture);
		TextureID		GetDiffuseTexture();
		void			SetMetallicTexture(TextureID metallicTexture);
		TextureID		GetMetallicTexture();
		void			SetEmissiveTexture(TextureID emissiveTexture);
		TextureID		GetEmissiveTexture();
		void			SetNormalTexture(TextureID normalTexture);
		TextureID		GetNormalTexture();

		PBRMaterialParameter& GetMaterialParameter();

		std::string GetName();
		
	private:
		std::string  m_Name;
		RTMaterialID m_RTMaterialID;

		TextureID	m_DiffuseTexture;
		TextureID	m_MetallicTexture;
		TextureID	m_EmissiveTexture;
		TextureID	m_NormalTexture;

		PBRMaterialParameter m_PBRParameter;
	};



	class MaterialManager
	{
	public:
		MaterialManager() = default;

		View<ptr<Material>> GetMaterials();
		opt<ptr<Material>>	GetMaterialByID(const MaterialID& id);
		MaterialID			CreateMaterial(ptr<GraphicsShader> shader, ptr<RenderBuffer> buffer,const std::string& name);
		MaterialID			CreateMaterial(ptr<GraphicsShader> shader, RenderAPI& api, const std::string& name);

		opt<ptr<RTMaterial>> GetRTMaterialByID(const RTMaterialID& id);
		RTMaterialID		 CreateRTMaterial(const std::string& name);

	private:
		// std::vector<ptr<RTMaterial>> m_RTMaterials;
		std::vector<ptr<Material>> m_Materials;
		std::unordered_map<MaterialID, ptr<Material>> m_MaterialIDTable;
		std::unordered_map<RTMaterialID, ptr<RTMaterial>> m_RTMaterialIDTable;
	};
}
