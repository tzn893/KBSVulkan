#include "Asset/AssetManager.h"

namespace kbs
{
	AssetManager::AssetManager()
	{
		m_MeshPool = std::make_shared<MeshPool>();
		m_MaterialManager = std::make_shared<MaterialManager>();
		m_ShaderManager = std::make_shared<ShaderManager>();
	}

	ptr<AssetManager> AssetManager::GetInstance()
	{
		if (g_Manager == nullptr)
		{
			g_Manager = std::make_shared<AssetManager>();
		}
		return g_Manager;
	}
	
	ptr<MeshPool> AssetManager::GetMeshPool()
	{
		return m_MeshPool;
	}

	ptr<MaterialManager> AssetManager::GetMaterialManager()
	{
		return m_MaterialManager;
	}

	ptr<ShaderManager> AssetManager::GetShaderManager()
	{
		return m_ShaderManager;
	}

	ptr<AssetManager> AssetManager::g_Manager = nullptr;
}