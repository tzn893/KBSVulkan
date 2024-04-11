#pragma once
#include <memory>
#include <string>

#include "Scene/UUID.h"
#include "Scene/entt/entt.h"
#include "Scene/Components.h"
#include "Common.h"

// copied mostly from hazel
namespace kbs
{
	class Entity;

	class Scene
	{
	public:
		Scene();
		~Scene();

		static ptr<Scene> Copy(ptr<Scene> other);

		Entity CreateMainCamera(const TransformComponent& trans, const CameraComponent& camera);
		Entity GetMainCamera();

		TransformComponent CreateTransform(opt<Entity> parent,const vec3& position, const quat& rot, const vec3& scale);

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);

		UUID   GetRootID();

		template<typename... Components>
		void IterateAllEntitiesWith(std::function<void(Entity e)> visiter)
		{
			for (auto e : m_Registry.view<Components...>())
			{
				visiter(Entity(e, this));
			}
		}

	private:
		entt::registry	m_Registry;
		std::unordered_map<UUID, entt::entity> m_EntityMap;
		UUID m_Root;
		opt<UUID> m_MainCamera ;

		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

}
