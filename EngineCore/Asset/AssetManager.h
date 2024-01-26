#pragma once
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Renderer/Shader.h"
#include "Asset/TextureManager.h"
#include "Core/Singleton.h"
#include "Asset/Model.h"


namespace kbs
{
	// TODO better way than singleton
	class AssetManager : public Is_Singleton
	{
	public:
		AssetManager();
		
		ptr<MeshPool>				GetMeshPool();
		ptr<MaterialManager>		GetMaterialManager();
		ptr<ShaderManager>			GetShaderManager();
		ptr<TextureManager>			GetTextureManager();

		ptr<ModelManager>			GetModelManager();

	private:
		ptr<MeshPool>			m_MeshPool;
		ptr<MaterialManager>	m_MaterialManager;
		ptr<ShaderManager>		m_ShaderManager;
		ptr<TextureManager>		m_TextureManager;
		ptr<ModelManager>		m_ModelManager;
	};
}