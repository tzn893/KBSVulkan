#pragma once
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Renderer/Shader.h"


namespace kbs
{
	class AssetManager
	{
	public:
		AssetManager();
		// TODO better way than singleton
		static ptr<AssetManager>	GetInstance();
		
		ptr<MeshPool>				GetMeshPool();
		ptr<MaterialManager>		GetMaterialManager();
		ptr<ShaderManager>			GetShaderManager();

	private:
		ptr<MeshPool>			m_MeshPool;
		ptr<MaterialManager>	m_MaterialManager;
		ptr<ShaderManager>		m_ShaderManager;

		static ptr<AssetManager> g_Manager;
	};
}