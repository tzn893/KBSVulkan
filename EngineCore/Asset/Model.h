#pragma once
#include "Scene/UUID.h"
#include "Asset/TextureManager.h"
#include "Renderer/Mesh.h"
#include "Math/math.h"
#include "Renderer/Shader.h"
#include "Scene/Components.h"
#include "Scene/Scene.h"
#include "Renderer/Material.h"

namespace kbs
{
    using ModelID = UUID;

    class Scene;

    class ModelMaterialSet
    {
    public:
        ModelMaterialSet(const std::vector<MaterialID>& materials, ShaderID targetShader, std::vector<RenderPassFlags> initialFlags) : m_Materials(materials),
            m_TargetShader(targetShader),m_MaterialRenderPassFlag(initialFlags) {}

        MaterialID GetModelMaterial(uint32_t idx);
        
        void            SetMaterialRenderPassFlag(RenderPassFlags flags, int32_t idx = -1);
        RenderPassFlags GetMaterialRenderPassFlag(uint32_t idx);
        void            AddMaterialRenderPassFlag(RenderPassFlags flags, int32_t idx = -1);


    private:
        std::vector<MaterialID>      m_Materials;
        std::vector<RenderPassFlags> m_MaterialRenderPassFlag;
        ShaderID                m_TargetShader;
    };

    struct ModelInstantiateOption
    {
        enum Flag
        {
            RayTracingSupport = 0x1,
        };
        uint64_t flags = 0;
    };

    struct ModelLoadOption
    {
        enum Flag
        {
            RayTracingSupport = 0x1,
            SkipOpaque = 0x2,
            SkipTransparent = 0x4
        };
        uint64_t flags = 0;
    };

    class Model
    {
    public:

        enum class AlphaMode
        {
            Opaque, Mask, Blend
        };

        struct MaterialSet
        {
            AlphaMode alphaMode;
            float metallicFactor;
            float roughnessFactor;
            vec4  baseColor;
            TextureID baseColorTexture;
            TextureID metallicRoughnessTexture;
            TextureID normalTexture;
            TextureID occlusionTexture;
            TextureID emissiveTexture;
            TextureID specularGlossinessTexture;
            TextureID diffuseTexture;
        };

        struct Primitive
        {
            MeshID      mesh;
            uint32_t    materialSetID;
        };

        struct Mesh
        {
            std::vector<uint32_t> primitiveID;
            std::string name;
            MeshGroupID meshGroupID;
            // vec3 translation;
            // vec3 scale;
            // quat rotation;
        };

        Model(std::vector<TextureID>      textureSet, std::vector<MaterialSet>    materialSet, std::vector<Primitive>      primitiveSet, std::vector<Model::Mesh>           meshSet, const std::string& name,
            ModelLoadOption option);
        
        opt<ptr<ModelMaterialSet>> CreateMaterialSetForModel(ShaderID targetShader, RenderAPI& api);

        Entity Instantiate(ptr<Scene> scene, std::string name, View<ptr<ModelMaterialSet>> materialSet,const TransformComponent& modelTrans, ModelInstantiateOption options);
        Entity Instantiate(ptr<Scene> scene, std::string name, ptr<ModelMaterialSet> materialSet, const TransformComponent& modelTrans, ModelInstantiateOption options = ModelInstantiateOption{});

    private:
        std::vector<TextureID>      m_TextureSet;
        std::vector<MaterialSet>    m_MaterialSet;
        std::vector<Primitive>      m_PrimitiveSet;
        std::vector<Mesh>           m_MeshSet;
        ModelID                     m_ModelID;
        std::string                 m_Name;

        bool                        m_SupportRayTracing : 1;
    };


    class ModelManager
    {
    public:
        ModelManager() = default;

        opt<ModelID> LoadFromGLTF(const std::string& path, RenderAPI& api, ModelLoadOption option);

        opt<ptr<Model>> GetModel(ModelID model);
        opt<ModelID> GetModelByPath(const std::string& path);

    private:
        std::unordered_map<ModelID, ptr<Model>> m_Models;
        std::unordered_map<std::string, ModelID> m_ModelPathTable;
    };
}