#include "Renderer/RTScene.h"
#include "Scene/Components.h"
#include "Scene/Entity.h"
#include "Scene/Transform.h"
#include "Asset/AssetManager.h"


namespace kbs
{
    SceneAccelerationStructure::SceneAccelerationStructure(ptr<gvk::TopAccelerationStructure> tlas):
        m_tlas(tlas)
    {
    }
    
    ptr<gvk::TopAccelerationStructure> SceneAccelerationStructure::GetTlas()
    {
        return m_tlas;
    }
    RTScene::RTScene(ptr<Scene> scene)
        :m_Scene(scene)
    {
    }

    ptr<SceneAccelerationStructure> RTScene::GetSceneAccelerationStructure()
    {
        return m_Slas;
    }


    static VkTransformMatrixKHR Mat4CastVKTransformMat(mat4 m)
    {
        VkTransformMatrixKHR vkm;
        vkm.matrix[0][0] = m[0][0], vkm.matrix[0][1] = m[0][1], vkm.matrix[0][2] = m[0][2], vkm.matrix[0][3] = m[0][3];
        vkm.matrix[1][0] = m[1][0], vkm.matrix[1][1] = m[1][1], vkm.matrix[1][2] = m[1][2], vkm.matrix[1][3] = m[1][3];
        vkm.matrix[2][0] = m[2][0], vkm.matrix[2][1] = m[2][1], vkm.matrix[2][2] = m[2][2], vkm.matrix[2][3] = m[2][3];

        return vkm;
    }

    void RTScene::UpdateSceneAccelerationStructure(RenderAPI api, RTSceneUpdateOption option)
    {
        KBS_ASSERT(m_Slas == nullptr, "currently update tlas dynamically is not supported");

        std::vector<gvk::GvkTopAccelerationStructureInstance> instances;

        ptr<MeshPool> meshPool = Singleton::GetInstance<AssetManager>()->GetMeshPool();
        ptr<MaterialManager> materialManager = Singleton::GetInstance<AssetManager>()->GetMaterialManager();

        std::unordered_map<UUID, uint64_t> materialSetIdxTable;
        std::unordered_map<UUID, int> textureSetIdxTable;

        if (m_Slas  == nullptr)
        {
            uint32_t customIdx = 0;

            m_Scene->IterateAllEntitiesWith<RayTracingGeometryComponent>(
                [&](Entity e)
                {
                    auto rtComponent = e.GetComponent<RayTracingGeometryComponent>();
                    auto renderableComponent = e.GetComponent<RenderableComponent>();
            
                    auto transform = Transform(e.GetComponent<TransformComponent>(), e);

                    mat4 modelMatrix = transform.GetObjectUBO().model;
                    ptr<MeshAccelerationStructure> meshAS;

                    if (auto var = meshPool->GetAccelerationStructure(renderableComponent.targetMesh, rtComponent.opaque);
                        var.has_value())
                    {
                        meshAS = var.value();
                    }
                    else
                    {
                        meshAS = meshPool->CreateAccelerationStructure(renderableComponent.targetMesh, api, rtComponent.opaque).value();
                    }

                    gvk::GvkTopAccelerationStructureInstance topInstance{};
                    topInstance.blas = meshAS->GetAS();
                    topInstance.instanceCustomIndex = customIdx++;
                    // TODO support mask
                    topInstance.trans = Mat4CastVKTransformMat(modelMatrix);
                    topInstance.mask = 0xff;
                    
                    instances.push_back(topInstance);

                    ptr<RTMaterial> mat = materialManager->GetRTMaterialByID(rtComponent.rayTracingMaterial).value();
                    
                    Mesh mesh = meshPool->GetMesh(renderableComponent.targetMesh).value();
                    MeshGroupID meshGroupID = mesh.GetMeshGroupID();
                    ptr<MeshGroup> meshGroup = meshPool->GetMeshGroup(meshGroupID).value();

                    uint64_t vertexAddress = meshGroup->GetVertexBuffer()->GetBuffer()->GetAddress();
                    uint64_t indexAddress = meshGroup->GetType() != MeshGroupType::Vertices ?
                        meshGroup->GetIndexBuffer()->GetBuffer()->GetAddress() : 0;
                    uint64_t materialSetIdx = 0;

                    if (materialSetIdxTable.count(rtComponent.rayTracingMaterial))
                    {
                        materialSetIdx = materialSetIdxTable[rtComponent.rayTracingMaterial];
                    }
                    else
                    {
                        RTMaterialSetDesc materialSet;

                        auto getTextureIdx = [&](UUID textureID, bool option)
                        {
                            if (!option || textureID.IsInvalid())
                            {
                                return -1;
                            }

                            if (!textureSetIdxTable.count(textureID))
                            {
                                m_TextureSet.push_back(textureID);
                                textureSetIdxTable[textureID] = m_TextureSet.size() - 1;
                            }
							return textureSetIdxTable[textureID];

                        };

						materialSet.diffuseTexIdx = getTextureIdx(mat->GetDiffuseTexture(), option.loadDiffuseTex);
                        materialSet.emissiveTexIdx = getTextureIdx(mat->GetEmissiveTexture(), option.loadEmissiveIdx);
                        materialSet.metallicTexIdx = getTextureIdx(mat->GetMetallicTexture(), option.loadMetallicRoughnessTex);
                        materialSet.normalTexIdx = getTextureIdx(mat->GetNormalTexture(), option.loadNormalTex);

                        materialSet.pbrParameters = mat->GetMaterialParameter();

                        m_MaterialSet.push_back(materialSet);
                        materialSetIdxTable[rtComponent.rayTracingMaterial] = m_MaterialSet.size() - 1;

                        materialSetIdx = m_MaterialSet.size() - 1;
                    }

                    RTObjectDesc objectDesc;
                    objectDesc.indexBufferAddress = indexAddress;
                    objectDesc.vertexBufferAddress = vertexAddress;
                    objectDesc.vertexOffset = mesh.GetVertexStart();
                    objectDesc.indexOffset = mesh.GetIndexStart();
                    objectDesc.entity = e;
                    objectDesc.materialSetIndex = materialSetIdx;

                    m_ObjDesc.push_back(objectDesc);
                }
            );
        }

        auto topAs = api.CreateTopAccelerationStructure(instances.data(), instances.size()).value();
        m_Slas = std::make_shared<SceneAccelerationStructure>(topAs);
    }

	std::vector<kbs::RTObjectDesc> RTScene::GetObjectDescs()
	{
        return m_ObjDesc;
	}

	std::vector<kbs::UUID> RTScene::GetTextureGroup()
	{
        return m_TextureSet;
	}

	std::vector<kbs::RTMaterialSetDesc> RTScene::GetMaterialSets()
	{
        return m_MaterialSet;
	}

}