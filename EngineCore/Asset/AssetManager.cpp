#include "Asset/AssetManager.h"

namespace kbs
{
	AssetManager::AssetManager()
	{
		m_MeshPool = std::make_shared<MeshPool>();
		m_MaterialManager = std::make_shared<MaterialManager>();
		m_ShaderManager = std::make_shared<ShaderManager>();
		m_TextureManager = std::make_shared<TextureManager>();
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
	ptr<TextureManager> AssetManager::GetTextureManager()
	{
		return m_TextureManager;
	}
}