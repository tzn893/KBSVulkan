#include "Mesh.h"
#include "Scene/Components.h"

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

}
