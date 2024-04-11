#pragma once
#include "Common.h"
#include "Scene/UUID.h"
#include "Renderer/RenderResource.h"
#include "Renderer/RenderAPI.h"

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

		ptr<RenderBuffer> GetVertexBuffer();
		ptr<RenderBuffer> GetIndexBuffer();

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

		uint32_t GetVertexStart();
		uint32_t GetIndexStart();
		uint32_t GetVertexCount();
		uint32_t GetIndexCount();
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

	class MeshAccelerationStructure
	{
	public:
		MeshAccelerationStructure() = default;

		ptr<gvk::BottomAccelerationStructure> GetAS();
		MeshID								  GetMesh();
	private:
		gvk::ptr<gvk::BottomAccelerationStructure> m_Blas;
		MeshID									   m_Mesh;
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

		opt<ptr<MeshAccelerationStructure>> GetAccelerationStructure(const MeshID& id, bool opaque);
		opt<ptr<MeshAccelerationStructure>>		CreateAccelerationStructure(const MeshID& id, RenderAPI api, bool opaque);
		opt<std::vector<ptr<MeshAccelerationStructure>>> CreateGroupAccelerationStructure(const MeshGroupID& id, RenderAPI api, bool opaque);

		void				RemoveMesh(MeshID id);
		void				RemoveMeshGroup(MeshGroupID id);

	private:

		opt<ptr<MeshAccelerationStructure>> CreateAccelerationStructureForVertexBuffer(RenderAPI api,const MeshID& id, ptr<RenderBuffer> vertexBuffer, ptr<RenderBuffer> indiceBuffer, bool opaque, VkIndexType indexType, uint32_t vertexPositionOffset, uint32_t vertexStride);
		void CopyMeshGroupVertexBuffer(RenderAPI api,const MeshGroupID& meshGroup, ptr<RenderBuffer>& vertexBuffer, ptr<RenderBuffer>& indexBuffer, uint32_t& vertexStride, uint32_t& vertexPositionOffset, VkIndexType& indexType);

		std::unordered_map<MeshID, ptr<MeshAccelerationStructure>> m_MeshAsOpaque;
		std::unordered_map<MeshID, ptr<MeshAccelerationStructure>> m_MeshAsTransparent;


		std::unordered_map<MeshID, Mesh> m_Meshs;
		std::unordered_map<MeshGroupID, ptr<MeshGroup>> m_MeshGroups;

	};

}