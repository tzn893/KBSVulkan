#include "Asset/Model.h"
#include "Core/FileSystem.h"
#include "Asset/GLTFLoader.h"
#include "Asset/AssetManager.h"
#include "Scene/Entity.h"

namespace kbs
{
	FileSystem<ModelManager>* GetModelManagerFile()
	{
		return Singleton::GetInstance<FileSystem<ModelManager>>();
	}


	opt<ModelID> ModelManager::LoadFromGLTF(const std::string& path, RenderAPI& api, ModelLoadOption option)
	{
		std::string absolutePath;
		if (auto var = GetModelManagerFile()->FindAbsolutePath(path);var.has_value())
		{
			absolutePath = var.value();
		}
		else
		{
			return std::nullopt;
		}

		if (m_ModelPathTable.count(absolutePath))
		{
			return m_ModelPathTable[absolutePath];
		}

		// TODO for skinned models vertices should not be pretransformed
		vkglTF::Model vkModel;
		vkModel.loadFromFile(absolutePath, vkglTF::PreTransformVertices);

		std::string fileName = std::filesystem::path(path).filename().string();

		std::vector<TextureID> textureSet;
		for (auto& tex : vkModel.textures)
		{
			textureSet.push_back(tex.uploadTexture(api));
		}

		auto getTextureID = [&](vkglTF::Texture* tex)
		{
			if (tex == nullptr)
			{
				return TextureID::Invalid();
			}
			return textureSet[tex - vkModel.textures.data()];
		};

		std::vector<Model::MaterialSet> materialSet;
		for (auto& mat : vkModel.materials)
		{
			Model::MaterialSet set;
			set.alphaMode = (Model::AlphaMode)mat.alphaMode;
			set.baseColor = mat.baseColorFactor;
			set.metallicFactor = mat.metallicFactor;
			set.roughnessFactor = mat.roughnessFactor;

			set.baseColorTexture = getTextureID(mat.baseColorTexture);
			set.diffuseTexture = getTextureID(mat.normalTexture);
			set.emissiveTexture = getTextureID(mat.emissiveTexture);
			set.metallicRoughnessTexture = getTextureID(mat.metallicRoughnessTexture);
			set.normalTexture = getTextureID(mat.normalTexture);
			set.occlusionTexture = getTextureID(mat.occlusionTexture);
			set.specularGlossinessTexture = getTextureID(mat.specularGlossinessTexture);
			materialSet.push_back(set);
		}

		uint32_t vertexBufferSize = vkModel.vertexCount * sizeof(ShaderStandardVertex);
		uint32_t indexBufferSize = vkModel.indexBuffer.size() * sizeof(uint32_t);

		VkBufferUsageFlags vertexUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			indexUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		if (kbs_contains_flags(option.flags, ModelLoadOption::RayTracingSupport))
		{
			vertexUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			indexUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		}

		ptr<RenderBuffer> vertexBuffer = api.CreateBuffer(vertexUsage, vertexBufferSize, GVK_HOST_WRITE_NONE);
		ptr<RenderBuffer> indexBuffer = api.CreateBuffer(indexUsage, indexBufferSize, GVK_HOST_WRITE_NONE);

		vertexBuffer->GetBuffer()->SetDebugName(fileName + ".vertex");
		indexBuffer->GetBuffer()->SetDebugName(fileName + ".index");

		api.UploadBuffer(indexBuffer, vkModel.indexBuffer.data(), indexBufferSize);
		api.UploadBuffer(vertexBuffer, vkModel.assambledVertexBuffer.data(), vertexBufferSize);

		ptr<MeshPool> meshPool = Singleton::GetInstance<AssetManager>()->GetMeshPool();
		MeshGroupID meshGroupID = meshPool->CreateMeshGroup(vertexBuffer, indexBuffer, vkModel.vertexCount, (uint32_t)vkModel.indexBuffer.size(), kbs::MeshGroupType::Indices_I32);

		std::vector<Model::Primitive>      primitiveSet;
		std::vector<Model::Mesh>           meshSet;

		std::vector<vkglTF::Node*> nodeStack;
		nodeStack.insert(nodeStack.end(), vkModel.nodes.begin(), vkModel.nodes.end());

		auto getMaterialIdx = [&](vkglTF::Material* mat)
		{
			return (uint32_t)(mat - vkModel.materials.data());
		};

		while (!nodeStack.empty())
		{
			vkglTF::Node* node = *nodeStack.rbegin();
			nodeStack.pop_back();
			
			bool skipOpaque = kbs_contains_flags(option.flags, ModelLoadOption::SkipOpaque);
			bool skipTransparent = kbs_contains_flags(option.flags, ModelLoadOption::SkipTransparent);

			if (node->mesh)
			{
				std::vector<uint32_t> meshPrimitiveId;
				for (auto prim : node->mesh->primitives)
				{

					Model::Primitive primitive;
					primitive.materialSetID = getMaterialIdx(&prim->material);

					if (skipOpaque && materialSet[primitive.materialSetID].alphaMode == Model::AlphaMode::Opaque)
					{
						continue;
					}
					if (skipTransparent && materialSet[primitive.materialSetID].alphaMode == Model::AlphaMode::Blend)
					{
						continue;
					}
					
					MeshID meshID = meshPool->CreateMeshFromGroup(meshGroupID, prim->firstVertex, prim->vertexCount, prim->firstIndex, prim->indexCount);
					primitive.mesh = meshID;

					primitiveSet.push_back(primitive);
					meshPrimitiveId.push_back(primitiveSet.size() - 1);
				}

				Model::Mesh mesh;
				mesh.meshGroupID = meshGroupID;
				mesh.name = node->mesh->name;
				mesh.primitiveID = meshPrimitiveId;

				meshSet.push_back(mesh);
			}
		}

		auto model = std::make_shared<Model>(textureSet, materialSet, primitiveSet, meshSet, 
			Singleton::GetInstance<FileSystem<Model>>()->GetFileName(path), option);
		
		ModelID modelID = UUID::GenerateUncollidedID(m_Models);
		m_Models[modelID] = model;
		m_ModelPathTable[path] = modelID;

		return modelID;
}

opt<ptr<Model>> ModelManager::GetModel(ModelID model)
{
	if (m_Models.count(model)) 
	{
		return m_Models[model];
	}
	return std::nullopt;
}

opt<ModelID> ModelManager::GetModelByPath(const std::string& path)
{
	std::string absolutePath;
	if (auto var = GetModelManagerFile()->FindAbsolutePath(path); var.has_value())
	{
		absolutePath = var.value();
	}
	else
	{
		return std::nullopt;
	}
	if (m_ModelPathTable.count(absolutePath))
	{
		return m_ModelPathTable[absolutePath];
	}
	return std::nullopt;
}

	MaterialID ModelMaterialSet::GetModelMaterial(uint32_t idx)
	{
		KBS_ASSERT(idx < m_Materials.size(), "idx out of range");
		return m_Materials[idx];
	}

	void ModelMaterialSet::SetMaterialRenderPassFlag(RenderPassFlags flags, int32_t idx)
	{
		KBS_ASSERT(idx < m_MaterialRenderPassFlag.size());
		if (idx < 0)
		{
			for (auto& flag : m_MaterialRenderPassFlag) flag = flags;
		}
		else
		{
			m_MaterialRenderPassFlag[idx] = flags;
		}

	}
	
	RenderPassFlags ModelMaterialSet::GetMaterialRenderPassFlag(uint32_t idx)
	{
		KBS_ASSERT(idx >= 0 && idx < m_MaterialRenderPassFlag.size());
		return m_MaterialRenderPassFlag[idx];
	}

	void ModelMaterialSet::AddMaterialRenderPassFlag(RenderPassFlags flags, int32_t idx)
	{
		KBS_ASSERT(idx < m_MaterialRenderPassFlag.size(), "idx must be less than render pass flag types");
		if (idx < 0)
		{
			for (auto& flag : m_MaterialRenderPassFlag) flag |= flags;
		}
		else
		{
			m_MaterialRenderPassFlag[idx] |= flags;
		}
	}
	
	Model::Model(std::vector<TextureID> textureSet, std::vector<MaterialSet> materialSet, std::vector<Primitive> primitiveSet, std::vector<Model::Mesh> meshSet, const std::string& name, ModelLoadOption option)
		:m_TextureSet(textureSet),m_MaterialSet(materialSet), m_PrimitiveSet(primitiveSet), m_MeshSet(meshSet),m_Name(name)
	{
		if (kbs_contains_flags(option.flags, ModelLoadOption::RayTracingSupport))
		{
			m_SupportRayTracing = true;
		}
	}

	opt<ptr<ModelMaterialSet>> Model::CreateMaterialSetForModel(ShaderID targetShaderID, RenderAPI& api)
	{
		AssetManager* assetManager = Singleton::GetInstance<AssetManager>();
		ptr<GraphicsShader> targetShader;
		
		if (auto var = assetManager->GetShaderManager()->Get(targetShaderID); var.has_value() && var.value()->IsGraphicsShader())
		{
			targetShader = std::static_pointer_cast<GraphicsShader>(var.value());
		}
		else
		{
			return std::nullopt;
		}
		auto& reflect =  targetShader->GetShaderReflection();
		
		std::vector<MaterialID> materials;
		std::vector<RenderPassFlags> flag;

		for (uint32_t i = 0;i < m_MaterialSet.size();i++)
		{
			std::string materialName = m_Name + "_" + std::to_string(i);
			MaterialID materialID = assetManager->GetMaterialManager()->CreateMaterial(targetShader, api, materialName);
			ptr<Material> material = assetManager->GetMaterialManager()->GetMaterialByID(materialID).value();
			
			auto getMaterialParameterAssignment = [&](std::vector<std::string> keywords, TextureID texID, TextureID defaultTextureID)
			{
				auto tex = assetManager->GetTextureManager()->GetTextureByID(texID);
				if (!tex.has_value())
				{
					tex = assetManager->GetTextureManager()->GetTextureByID(defaultTextureID);
				}
				
				
				return [keywords, material, tex](const std::string& name, ShaderReflection::TextureInfo& _)
				{
					if (!tex.has_value())
					{
						return false;
					}

					for (auto& kw : keywords)
					{
						if (string_contains(lower_string(name), lower_string(kw)))
						{
							material->SetTexture(name, tex.value());
							return false;
						}
					}
					return true;
				};
			};

			auto getMaterialVariableAssignment = [&](std::vector<std::string> keywords, vec4 value)
			{
				return [keywords,material, value](const std::string& name, ShaderReflection::VariableInfo& varInfo)
				{
					for (auto& kw : keywords)
					{
						if (string_contains(lower_string(name), lower_string(kw)))
						{
							// TODO better way setting variables
							// duplicated shader variable iteration
							switch (varInfo.type)
							{
							case ShaderReflection::VariableType::Float:
								material->SetFloat(name, value.x);
								break;
							case ShaderReflection::VariableType::Float2:
								material->SetVec2(name, vec2(value.x, value.y));
								break;
							case ShaderReflection::VariableType::Float3:
								material->SetVec3(name, vec3(value.x, value.y, value.z));
								break;
							case ShaderReflection::VariableType::Float4:
								material->SetVec4(name, value);
								break;
							}
							return false;
						}
					}
					return true;
				};
			};
			auto texManager = assetManager->GetTextureManager();

			reflect.IterateTextures(getMaterialParameterAssignment({"baseColor","albedo"}, m_MaterialSet[i].baseColorTexture, texManager->GetDefaultWhite()));
			reflect.IterateTextures(getMaterialParameterAssignment({"diffuse"}, m_MaterialSet[i].diffuseTexture, texManager->GetDefaultWhite()));
			reflect.IterateTextures(getMaterialParameterAssignment({"metallic", "roughness"}, m_MaterialSet[i].metallicRoughnessTexture, texManager->GetDefaultWhite()));
			reflect.IterateTextures(getMaterialParameterAssignment({"emissive"}, m_MaterialSet[i].emissiveTexture, texManager->GetDefaultBlack()));
			reflect.IterateTextures(getMaterialParameterAssignment({ "normal" }, m_MaterialSet[i].normalTexture, texManager->GetDefaultNormal()));

			reflect.IterateVariables(getMaterialVariableAssignment({ "baseColor", "albedo" }, m_MaterialSet[i].baseColor));
			reflect.IterateVariables(getMaterialVariableAssignment({ "metallic" }, vec4(m_MaterialSet[i].metallicFactor)));
			reflect.IterateVariables(getMaterialVariableAssignment({ "roughness" }, vec4(m_MaterialSet[i].roughnessFactor)));
			
			materials.push_back(materialID);
		}

		for (uint32_t i = 0;i < m_MaterialSet.size();i++)
		{
			flag.push_back(m_MaterialSet[i].alphaMode == AlphaMode::Opaque ? RenderPass_Opaque : RenderPass_Transparent);
		}

		return std::make_shared<ModelMaterialSet>(materials, targetShaderID, flag);
	}

	Entity Model::Instantiate(ptr<Scene> scene, std::string name, View<ptr<ModelMaterialSet>> materialSets, const TransformComponent& modelTrans, ModelInstantiateOption options)
	{
		Entity entity = scene->CreateEntity(name);
		entity.AddComponent<TransformComponent>(modelTrans);

		ptr<MeshPool> meshPool = Singleton::GetInstance<AssetManager>()->GetMeshPool();

		for (auto& mesh : m_MeshSet)
		{
			TransformComponent comp;
			comp.parent = entity.GetUUID();
			comp.position = vec3(0, 0, 0);
			comp.rotation = quat();
			comp.scale = vec3(1, 1, 1);

			std::string meshName = name + "_" + mesh.name;
			for (auto& primID : mesh.primitiveID)
			{
				std::string primitiveName = meshName + "_" + std::to_string(primID);
				Primitive& prim = m_PrimitiveSet[primID];
				Model::MaterialSet& modelMaterialSet = m_MaterialSet[prim.materialSetID];

				RenderableComponent renderableComp;
				//renderableComp.targetMaterial = materialSet->GetModelMaterial(prim.materialSetID);
				renderableComp.targetMesh = prim.mesh;
				for (auto& materialSet : materialSets)
				{
					renderableComp.AddRenderablePass(materialSet->GetModelMaterial(prim.materialSetID),
						materialSet->GetMaterialRenderPassFlag(prim.materialSetID));
				}
			
				Entity subMeshEntity = scene->CreateEntity(primitiveName);
				subMeshEntity.AddComponent<TransformComponent>(comp);
				subMeshEntity.AddComponent<RenderableComponent>(renderableComp);

				if (kbs_contains_flags(options.flags, ModelInstantiateOption::RayTracingSupport))
				{
					KBS_ASSERT(m_SupportRayTracing, "Only model loaded with RayTracingSupport flag can be instantiated by ray tracing support");
					RayTracingGeometryComponent rtGeom;
					rtGeom.opaque = modelMaterialSet.alphaMode == AlphaMode::Opaque;
						
					std::string rtMaterialSetName = primitiveName + "_RTMaterial";
					AssetManager* assetManager = Singleton::GetInstance<AssetManager>();
					auto materialManager = assetManager->GetMaterialManager();
					
				    auto rtMaterialID = materialManager->CreateRTMaterial(rtMaterialSetName);
					auto rtMaterial = materialManager->GetRTMaterialByID(rtMaterialID).value();

					rtMaterial->SetDiffuseTexture(modelMaterialSet.baseColorTexture);
					rtMaterial->SetEmissiveTexture(modelMaterialSet.emissiveTexture);
					rtMaterial->SetMetallicTexture(modelMaterialSet.metallicRoughnessTexture);
					rtMaterial->SetNormalTexture(modelMaterialSet.normalTexture);

					PBRMaterialParameter& pbr = rtMaterial->GetMaterialParameter();
					pbr.albedo = modelMaterialSet.baseColor;
					pbr.metallic = modelMaterialSet.metallicFactor;
					pbr.roughness = modelMaterialSet.roughnessFactor;


					rtGeom.rayTracingMaterial = rtMaterialID;
					subMeshEntity.AddComponent<RayTracingGeometryComponent>(rtGeom);
				}
			}
		}

		return entity;
	}
	Entity Model::Instantiate(ptr<Scene> scene, std::string name, ptr<ModelMaterialSet> materialSet, const TransformComponent& modelTrans, ModelInstantiateOption options)
	{
		return Instantiate(scene, name, View(&materialSet, 1), modelTrans, options);
	}
}