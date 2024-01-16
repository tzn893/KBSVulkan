#pragma once
#include "Shader.h"
#include "Math/math.h"
#include "Renderer/RenderResource.h"
#include "Scene/UUID.h"
#include "Renderer/RenderAPI.h"

namespace kbs
{
	using MaterialID = UUID;

	class Material
	{
	public:
		Material(const UUID& uuid,const std::string& name,ptr<GraphicsShader> shader, ptr<RenderBuffer> buffer);
		
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
		std::string				m_Name;
		UUID					m_ID;
		ptr<GraphicsShader>		m_Shader;
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

	class MaterialManager
	{
	public:
		MaterialManager() = default;

		View<ptr<Material>> GetMaterials();
		opt<ptr<Material>>	GetMaterialByID(const MaterialID& id);
		MaterialID			CreateMaterial(ptr<GraphicsShader> shader, ptr<RenderBuffer> buffer,const std::string& name);
		MaterialID			CreateMaterial(ptr<GraphicsShader> shader, RenderAPI& api, const std::string& name);

	private:
		std::vector<ptr<Material>> m_Materials;
		std::unordered_map<MaterialID, ptr<Material>> m_MaterialIDTable;
	};
}
