#include "Mesh.h"
#include "Scene/Components.h"
#include "Renderer/Shader.h"

namespace kbs
{
    opt<Mesh> kbs::MeshPool::GetMesh(const MeshID& id)
    {
        if (m_Meshs.count(id))
        {
            return m_Meshs[id];
        }

        return std::nullopt;
    }

    opt<ptr<MeshGroup>> kbs::MeshPool::GetMeshGroup(const MeshGroupID& id)
    {
        if (m_MeshGroups.count(id));
        {
            return m_MeshGroups[id];
        }

        return std::nullopt;
    }

    MeshGroupID kbs::MeshPool::CreateMeshGroup(ptr<RenderBuffer> vertices, uint32_t verticesCount)
    {
        ptr<MeshGroup> meshGroup = std::make_shared<MeshGroup>();
        meshGroup->m_Vertices = vertices;
        meshGroup->m_VerticesCount = verticesCount;
        meshGroup->m_IndicesCount = 0;
        meshGroup->m_MeshGroupType = MeshGroupType::Vertices;

        MeshGroupID id = UUID::GenerateUncollidedID(m_MeshGroups);
        m_MeshGroups[id] = meshGroup;
        meshGroup->m_MeshGroupID = id;
        meshGroup->m_Pool = this;

        return id;
    }

    MeshGroupID kbs::MeshPool::CreateMeshGroup(ptr<RenderBuffer> vertices, ptr<RenderBuffer> indices, uint32_t verticesCount, uint32_t indicesCount, MeshGroupType type)
    {
        KBS_ASSERT(type != MeshGroupType::Indices_I16 || type != MeshGroupType::Indices_I32, "indices group must be created either Indices_I16 or Indices_I32");
        uint32_t indiceSize = MeshGroupType::Indices_I16 == type ? sizeof(uint16_t) : sizeof(uint32_t);
        KBS_ASSERT(indiceSize * indicesCount == indices->GetBuffer()->GetSize(), "incompatiable buffer size expected {} * {} = {}, actual {}",
            indiceSize, indicesCount, indiceSize * indicesCount, indices->GetBuffer()->GetSize());

        ptr<MeshGroup> meshGroup = std::make_shared<MeshGroup>();
        
        meshGroup->m_Vertices = vertices;
        meshGroup->m_Indices = indices;
        meshGroup->m_VerticesCount = verticesCount;
        meshGroup->m_IndicesCount = indicesCount;
        meshGroup->m_MeshGroupType = type;

        MeshGroupID id = UUID::GenerateUncollidedID(m_MeshGroups);
        m_MeshGroups[id] = meshGroup;
        meshGroup->m_MeshGroupID = id;
        meshGroup->m_Pool = this;

        return id;
    }

    MeshID kbs::MeshPool::CreateMeshFromGroup(MeshGroupID groupID, uint32_t verticesStart, uint32_t verticesCount, uint32_t indicesStart, uint32_t indicesCount)
    {
        auto meshGroup = GetMeshGroup(groupID);
        KBS_ASSERT(meshGroup.has_value(), "invalid mesh group id");

        KBS_ASSERT(meshGroup.value()->m_VerticesCount >= verticesStart + verticesCount, " vertex out of boundary: vertices offset {} vertices count {}, mesh group vertices count {}", 
            verticesStart, verticesCount, meshGroup.value()->m_VerticesCount);
        KBS_ASSERT(meshGroup.value()->m_IndicesCount >= indicesStart + indicesCount, " index out of boundary: indices offset {} indices count {}, mesh group indices count {}",
            indicesStart, indicesCount, meshGroup.value()->m_IndicesCount);

        Mesh mesh;
        mesh.m_IndicesCount = indicesCount;
        mesh.m_IndicesStart = indicesStart;
        mesh.m_VerticesCount = verticesCount;
        mesh.m_VerticesStart = verticesStart;
        mesh.m_Pool = this;
        mesh.m_GroupID = groupID;
        mesh.m_Id = UUID::GenerateUncollidedID(m_Meshs);

        m_Meshs[mesh.m_Id] = mesh;
        meshGroup.value()->m_SubMeshes.push_back(mesh.m_Id);

        return mesh.m_Id;
    }

    opt<ptr<MeshAccelerationStructure>> MeshPool::GetAccelerationStructure(const MeshID& id, bool opaque)
    {
        if (opaque)
        {
            if (m_MeshAsOpaque.count(id))
            {
                return m_MeshAsOpaque[id];
            }
        }
        else
        {
            if (m_MeshAsTransparent.count(id))
            {
                return m_MeshAsTransparent[id];
            }
        }

        return std::nullopt;
    }

    opt<ptr<MeshAccelerationStructure>> MeshPool::CreateAccelerationStructure(const MeshID& id, RenderAPI api, bool opaque)
    {
        KBS_ASSERT(m_Meshs.count(id), "invalid mesh id");
        if (auto var = GetAccelerationStructure(id, opaque); var.has_value())
        {
            return var.value();
        }

        MeshGroupID meshGroupID = m_Meshs[id].m_GroupID;
        ptr<RenderBuffer> vertexBuffer;
        ptr<RenderBuffer> indexBuffer;
        uint32_t vertexStride, vertexPositionOffset;
        VkIndexType indexType;
        
        CopyMeshGroupVertexBuffer(api, meshGroupID, vertexBuffer, indexBuffer, vertexStride, vertexPositionOffset, indexType);

        if (auto var = CreateAccelerationStructureForVertexBuffer(api, id, vertexBuffer, indexBuffer, opaque,
            indexType, vertexPositionOffset, vertexStride); var.has_value())
        {
            if (opaque)
            {
                m_MeshAsOpaque[id] = var.value();
            }
            else
            {
                m_MeshAsTransparent[id] = var.value();
            }
            return var.value();
        }

        return std::nullopt;
    }

    opt<std::vector<ptr<MeshAccelerationStructure>>> MeshPool::CreateGroupAccelerationStructure(const MeshGroupID& id, RenderAPI api, bool opaque)
    {
        KBS_ASSERT(m_MeshGroups.count(id), "invalid mesh group id");
        
        ptr<MeshGroup> meshGroup = m_MeshGroups[id];
        auto& subMeshes = meshGroup->m_SubMeshes;

        ptr<RenderBuffer> vertexBuffer;
        ptr<RenderBuffer> indexBuffer;
        uint32_t vertexStride, vertexPositionOffset;
        VkIndexType indexType;

        CopyMeshGroupVertexBuffer(api, id, vertexBuffer, indexBuffer, vertexStride, vertexPositionOffset, indexType);

        std::vector<ptr<MeshAccelerationStructure>> accelStructures;
        for (auto meshID : subMeshes)
        {
            if (auto var = GetAccelerationStructure(meshID, opaque); var.has_value())
            {
                accelStructures.push_back(var.value());
                continue;
            }

            auto var = CreateAccelerationStructureForVertexBuffer(api, meshID, vertexBuffer, indexBuffer, opaque,
                indexType, vertexPositionOffset, vertexStride);
            if (var.has_value())
            {
                accelStructures.push_back(var.value());
            }
            else
            {
                return std::nullopt;
            }
        }

        for (uint32_t i = 0;i < subMeshes.size();i++)
        {
            if (opaque)
            {
                m_MeshAsOpaque[subMeshes[i]] = accelStructures[i];
            }
            else
            {
                m_MeshAsTransparent[subMeshes[i]] = accelStructures[i];
            }
        }

        return accelStructures;
    }

    void kbs::MeshPool::RemoveMesh(MeshID id)
    {
        auto mesh = GetMesh(id);
        if (!mesh.has_value())
        {
            return;
        }

        m_Meshs.erase(m_Meshs.find(id));
        auto meshGroup = GetMeshGroup(mesh.value().m_GroupID);
        KBS_ASSERT(meshGroup.has_value(), "invalid mesh group id for mesh");
        auto& meshGroupSubMeshes = meshGroup.value()->m_SubMeshes;

        meshGroupSubMeshes.erase(std::find(meshGroupSubMeshes.begin(), meshGroupSubMeshes.end(), id));
    }

    void kbs::MeshPool::RemoveMeshGroup(MeshGroupID id)
    {
        auto meshGroup = GetMeshGroup(id);
        if (!meshGroup.has_value())
        {
            return;
        }

        m_MeshGroups.erase(m_MeshGroups.find(id));
        for (auto& mesh : meshGroup.value()->m_SubMeshes)
        {
            m_Meshs.erase(m_Meshs.find(mesh));
        }
    }

    opt<ptr<MeshAccelerationStructure>> MeshPool::CreateAccelerationStructureForVertexBuffer(RenderAPI api, const MeshID& id, ptr<RenderBuffer> vertexBuffer, ptr<RenderBuffer> indiceBuffer, bool opaque, VkIndexType indexType, uint32_t vertexPositionOffset, uint32_t vertexStride)
    {
        Mesh& mesh = m_Meshs[id];

        gvk::GvkBottomAccelerationStructureGeometryTriangles structure{};
        structure.flags = opaque ? VK_GEOMETRY_OPAQUE_BIT_KHR : 0;
    
        structure.indexCount = mesh.m_IndicesCount;
        structure.indexOffset = mesh.m_IndicesStart;
        structure.indiceBuffer = indiceBuffer->GetBuffer();
        structure.indiceType = indexType;
        structure.maxVertex = mesh.m_VerticesStart + mesh.m_VerticesCount;
        structure.transformOffset = 0;
        structure.vertexBuffer = vertexBuffer->GetBuffer();
        structure.vertexPositionAttributeOffset = vertexPositionOffset;
        structure.vertexStride = vertexStride;

        structure.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        auto blas = api.CreateBottomAccelerationStructure(&structure, 1);
        if (!blas.has_value()) return std::nullopt;

        ptr<MeshAccelerationStructure> meshAs = std::make_shared<MeshAccelerationStructure>();
        meshAs->m_Mesh = id;
        meshAs->m_Blas = blas.value();

        return meshAs;
    }

    void MeshPool::CopyMeshGroupVertexBuffer(RenderAPI api,const MeshGroupID& meshGroupID, ptr<RenderBuffer>& vertexBuffer, ptr<RenderBuffer>& indexBuffer, uint32_t& vertexStride, uint32_t& vertexPositionOffset, VkIndexType& indexType)
    {
        ptr<MeshGroup> meshGroup = m_MeshGroups[meshGroupID];
        ptr<gvk::Buffer> meshGroupVertexBuffer = meshGroup->m_Vertices->GetBuffer();
        ptr<gvk::Buffer> meshGroupIndexBuffer = meshGroup->m_Indices->GetBuffer();

        KBS_ASSERT(meshGroup->m_VerticesCount * sizeof(ShaderStandardVertex) == meshGroupVertexBuffer->GetSize(),
            "currently we only support mesh with ShaderStandardVertex as input of CreateAccelertaionStructure");


        uint32_t vertexBufferSize = meshGroup->m_Vertices->GetBuffer()->GetSize();
        vertexBuffer = api.CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            vertexBufferSize,
            GVK_HOST_WRITE_NONE);

        uint32_t indexBufferSize = 0;
        bool hasIndexBuffer = meshGroup->m_MeshGroupType != MeshGroupType::Vertices;
        if (hasIndexBuffer)
        {
            indexBufferSize = meshGroup->m_Indices->GetBuffer()->GetSize();
            indexBuffer = api.CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                indexBufferSize,
                GVK_HOST_WRITE_NONE);
        }

        api.CopyRenderBuffer(meshGroup->m_Vertices, vertexBuffer);
        api.CopyRenderBuffer(meshGroup->m_Indices, indexBuffer);

        // TODO support user defined vertices 
        vertexStride = sizeof(ShaderStandardVertex);
        vertexPositionOffset = 0;
        
        switch (meshGroup->GetType())
        {
        case MeshGroupType::Indices_I16:
            indexType = VK_INDEX_TYPE_UINT16;
            break;
        case MeshGroupType::Indices_I32:
            indexType = VK_INDEX_TYPE_UINT32;
            break;
        default:
            indexType = VK_INDEX_TYPE_NONE_KHR;
        }
    }

    MeshGroupID MeshGroup::GetID()
    {
        return m_MeshGroupID;
    }

    MeshGroupType MeshGroup::GetType()
    {
        return m_MeshGroupType;
    }

    void MeshGroup::BindVertexBuffer(VkCommandBuffer cmd)
    {
        VkBuffer vertBuffers[] = { m_Vertices->GetBuffer()->GetBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, vertBuffers, offsets);

        switch (m_MeshGroupType)
        {
        case MeshGroupType::Indices_I16:
            vkCmdBindIndexBuffer(cmd, m_Indices->GetBuffer()->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);
            break;
        case MeshGroupType::Indices_I32:
            vkCmdBindIndexBuffer(cmd, m_Indices->GetBuffer()->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
            break;
        }
    }

	kbs::ptr<kbs::RenderBuffer> MeshGroup::GetVertexBuffer()
	{
        return m_Vertices;
	}

	kbs::ptr<kbs::RenderBuffer> MeshGroup::GetIndexBuffer()
	{
        return m_Indices;
	}

	MeshID Mesh::GetMeshID()
    {
        return m_Id;
    }

    MeshGroupID Mesh::GetMeshGroupID()
    {
        return m_GroupID;
    }

    void Mesh::Draw(VkCommandBuffer cmd, uint32_t instanceCount)
    {
        auto meshGroup = m_Pool->GetMeshGroup(m_GroupID);
        KBS_ASSERT(meshGroup.has_value(), " invalid id for mesh");

        switch (meshGroup.value()->GetType())
        {
        case MeshGroupType::Vertices:
            vkCmdDraw(cmd, m_VerticesCount, instanceCount, m_VerticesStart, 0);
            break;
        case MeshGroupType::Indices_I16:
        case MeshGroupType::Indices_I32:
            vkCmdDrawIndexed(cmd, m_IndicesCount, instanceCount, m_IndicesStart, 0, 0);
            break;
        }
    }

	uint32_t Mesh::GetVertexStart()
	{
        return m_VerticesStart;
	}

	uint32_t Mesh::GetIndexStart()
	{
        return m_IndicesStart;
	}

	uint32_t Mesh::GetVertexCount()
	{
        return m_VerticesCount;
	}

	uint32_t Mesh::GetIndexCount()
	{
        return m_IndicesCount;
	}

	ptr<gvk::BottomAccelerationStructure> MeshAccelerationStructure::GetAS()
    {
        return m_Blas;
    }

    MeshID MeshAccelerationStructure::GetMesh()
    {
        return m_Mesh;
    }

}
