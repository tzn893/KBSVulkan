#include "Entity.h"

namespace kbs {

	Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{}

	Scene* Entity::GetScene()
	{
		return m_Scene;
	}

}
