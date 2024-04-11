#pragma once
#include "gvk.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Common.h"
#include "Renderer/RenderAPI.h"
#include "Renderer/Material.h"


namespace kbs
{
    class SceneAccelerationStructure
    {
    public:
        SceneAccelerationStructure(ptr<gvk::TopAccelerationStructure> tlas);
        
        ptr<gvk::TopAccelerationStructure> GetTlas();

    private:
        ptr<gvk::TopAccelerationStructure> m_tlas;

    };

    struct RTObjectDesc
    {
        uint64_t vertexBufferAddress;
        uint64_t indexBufferAddress;
        uint64_t materialSetIndex;
        uint32_t vertexOffset;
        uint32_t indexOffset;
        Entity   entity;
    };

    struct RTMaterialSetDesc
    {
        int diffuseTexIdx;
        int normalTexIdx;
        int metallicTexIdx;
        int emissiveTexIdx;

        PBRMaterialParameter pbrParameters;
    };

    struct RTSceneUpdateOption
    {
        bool loadDiffuseTex : 1;
        bool loadNormalTex : 1;
        bool loadMetallicRoughnessTex : 1;
        bool loadEmissiveIdx : 1;

        RTSceneUpdateOption()
        {
            loadDiffuseTex = false;
            loadNormalTex = false;
            loadMetallicRoughnessTex = false;
            loadEmissiveIdx = false;
        }
    };


    class RTScene
    {
    public:
        RTScene(ptr<Scene> scene);
        
        ptr<SceneAccelerationStructure> GetSceneAccelerationStructure();
        
        void UpdateSceneAccelerationStructure(RenderAPI api, RTSceneUpdateOption option = RTSceneUpdateOption{});

        std::vector<RTObjectDesc>       GetObjectDescs();
        std::vector<UUID>               GetTextureGroup();
        std::vector<RTMaterialSetDesc>  GetMaterialSets();

    private:
        ptr<Scene> m_Scene;
        ptr<SceneAccelerationStructure> m_Slas;

        std::vector<RTObjectDesc> m_ObjDesc;
        std::vector<UUID>    m_TextureSet;
        std::vector<RTMaterialSetDesc> m_MaterialSet;
    };
}

