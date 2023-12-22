#pragma once
#include <memory>
#include <string>

#include "Scene/UUID.h"
#include "Scene/Entity.h"
#include "Scene/entt/entt.h"
#include "Common.h"

// copied mostly from hazel
namespace kbs
{
	class Entity;


	class Entity;

	class Scene
	{
	public:
		Scene();
		~Scene();

		static ptr<Scene> Copy(ptr<Scene> other);

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		Entity DuplicateEntity(Entity entity);

		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

	private:
		entt::registry m_Registry;
		std::unordered_map<UUID, entt::entity> m_EntityMap;

		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

}
