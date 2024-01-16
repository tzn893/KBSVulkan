#pragma once
#include "Common.h"
#include "Scene/UUID.h"
#include "Renderer/RenderResource.h"


namespace kbs
{
	using MeshID = UUID;
	using MeshGroupID = UUID;

	class MeshPool;

	enum class MeshGroupType
	{
		Vertices,
		Indices_I16,
		Indices_I32,
	};

	class RenderableComponent;

	class MeshGroup
	{
	public:
		MeshGroup() = default;

		MeshGroupID		GetID();
		MeshGroupType	GetType();
		void BindVertexBuffer(VkCommandBuffer cmd);
		~MeshGroup() = default;

	private:
		MeshGroupID				m_MeshGroupID;
		uint32_t				m_VerticesCount;
		uint32_t				m_IndicesCount;

		std::vector<MeshID>		m_SubMeshes;
		ptr<RenderBuffer>		m_Vertices;
		ptr<RenderBuffer>		m_Indices;
		MeshGroupType			m_MeshGroupType;

		MeshPool*				m_Pool;
		friend class MeshPool;
	};

	class Mesh
	{
	public:
		Mesh() = default;
		Mesh(const Mesh&) = default;

		MeshID		GetMeshID();
		MeshGroupID	GetMeshGroupID();

		void		Draw(VkCommandBuffer cmd, uint32_t instanceCount);
	
	private:
		MeshID m_Id;
		MeshGroupID m_GroupID;

		uint32_t m_VerticesStart;
		uint32_t m_VerticesCount;
		uint32_t m_IndicesStart;
		uint32_t m_IndicesCount;

		MeshPool* m_Pool;
		friend class MeshPool;
	};

	class MeshPool
	{
	public:
		MeshPool() = default;

		opt<Mesh>			GetMesh(const MeshID& id);
		opt<ptr<MeshGroup>> GetMeshGroup(const MeshGroupID& id);

		MeshGroupID			CreateMeshGroup(ptr<RenderBuffer> vertices, uint32_t verticesCount);
		MeshGroupID			CreateMeshGroup(ptr<RenderBuffer> vertices, ptr<RenderBuffer> indices, uint32_t verticesCount, uint32_t indicesCount, MeshGroupType type);

		MeshID				CreateMeshFromGroup(MeshGroupID id, uint32_t verticesStart, uint32_t verticesCount, 
			uint32_t indicesStart, uint32_t indicesCount);

		void				RemoveMesh(MeshID id);
		void				RemoveMeshGroup(MeshGroupID id);

	private:
		std::unordered_map<MeshID, Mesh> m_Meshs;
		std::unordered_map<MeshGroupID, ptr<MeshGroup>> m_MeshGroups;

	};

}