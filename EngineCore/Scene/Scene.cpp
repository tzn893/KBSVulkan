#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Scene/UUID.h"
#include "Scene/Entity.h"

/*
kbs::opt<kbs::Entity> kbs::Scene::FindEntityByName(std::string_view name)
{
    decs::EntityID entity;
    bool founded = false;

    auto name_iterator = [&](EntitySelfComponent& selfComp, NameComponent& nameComp)
    {
        if (nameComp.name == name)
        {
            entity = selfComp.self;
            founded = true;
        }
    };

    m_ecsWorld.for_each(name_iterator);
    
    if (founded)
    {
        return Entity(entity, this);
    }
    return std::nullopt;
}
*/

namespace kbs {

	Scene::Scene()
	{
		Entity rootEntity =  { m_Registry.create(), this };
		m_Root = UUID::GenerateUncollidedID(m_EntityMap);
		rootEntity.AddComponent<IDComponent>(m_Root);

		TransformComponent comp(vec3(), quat(), vec3(1., 1., 1.));
		comp.parent = m_Root;
		rootEntity.AddComponent<TransformComponent>(comp);
	}

	Scene::~Scene()
	{
	}

	template<typename... Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		([&]()
			{
				auto view = src.view<Component>();
		for (auto srcEntity : view)
		{
			entt::entity dstEntity = enttMap.at(src.get<IDComponent>(srcEntity).ID);

			auto& srcComponent = src.get<Component>(srcEntity);
			dst.emplace_or_replace<Component>(dstEntity, srcComponent);
		}
			}(), ...);
	}

	template<typename... Component>
	static void CopyComponent(ComponentGroup<Component...>, entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		CopyComponent<Component...>(dst, src, enttMap);
	}

	template<typename... Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		([&]()
			{
				if (src.HasComponent<Component>())
				dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
			}(), ...);
	}

	template<typename... Component>
	static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
	{
		CopyComponentIfExists<Component...>(dst, src);
	}

	ptr<Scene> Scene::Copy(ptr<Scene> other)
	{
		ptr<Scene> newScene = std::make_shared<Scene>();

		auto& srcSceneRegistry = other->m_Registry;
		auto& dstSceneRegistry = newScene->m_Registry;
		std::unordered_map<UUID, entt::entity> enttMap;

		// Create entities in new scene
		auto idView = srcSceneRegistry.view<IDComponent>();
		for (auto e : idView)
		{
			UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
			const auto& name = srcSceneRegistry.get<NameComponent>(e).name;
			Entity newEntity = newScene->CreateEntityWithUUID(uuid, name);
			enttMap[uuid] = (entt::entity)newEntity.m_EntityHandle;
		}

		// Copy components (except IDComponent and TagComponent)
		CopyComponent(AllCopiableComponents{}, dstSceneRegistry, srcSceneRegistry, enttMap);

		return newScene;
	}

	kbs::Entity Scene::CreateMainCamera(const TransformComponent& trans, const CameraComponent& camera)
	{
		KBS_ASSERT(!m_MainCamera.has_value(), "one main camera can be created for a scene");
		Entity mainCameraEntity = CreateEntity("MainCamera");
		m_MainCamera = mainCameraEntity.GetUUID();

		mainCameraEntity.AddComponent<TransformComponent>(trans);
		mainCameraEntity.AddComponent<CameraComponent>(camera);

		return mainCameraEntity;
	}

	kbs::Entity Scene::GetMainCamera()
	{
		KBS_ASSERT(m_MainCamera.has_value(), "main camera must be created");
		return GetEntityByUUID(m_MainCamera.value());
	}

	kbs::TransformComponent Scene::CreateTransform(opt<Entity> parent, const vec3& position, const quat& rot, const vec3& scale)
	{
		UUID root = m_Root;
		if (parent.has_value())
		{
			root = parent->GetUUID();
		}

		TransformComponent comp(position, rot, scale);
		comp.parent = root;
		return comp;
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateEntityWithUUID(UUID::GenerateUncollidedID(m_EntityMap), name);
	}

	Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		auto& nameComp = entity.AddComponent<NameComponent>();
		nameComp.name = name.empty() ? "Entity" : name;

		m_EntityMap[uuid] = entity;

		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_EntityMap.erase(entity.GetUUID());
		m_Registry.destroy(entity);
	}

	Entity Scene::FindEntityByName(std::string_view name)
	{
		auto view = m_Registry.view<NameComponent>();
		for (auto entity : view)
		{
			const NameComponent& tc = view.get<NameComponent>(entity);
			if (tc.name == name)
				return Entity{ entity, this };
		}
		return {};
	}

	Entity Scene::GetEntityByUUID(UUID uuid)
	{
		// TODO(Yan): Maybe should be assert
		if (m_EntityMap.find(uuid) != m_EntityMap.end())
			return { m_EntityMap.at(uuid), this };

		return {};
	}
	UUID Scene::GetRootID()
	{
		return m_Root;
	}
}

