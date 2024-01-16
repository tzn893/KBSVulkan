#pragma once

#include "Common.h"
#include "UUID.h"
#include "Math/math.h"



namespace kbs
{
	struct IDComponent
	{
		UUID ID;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
	};

	struct NameComponent
	{
		std::string name;

		NameComponent() = default;
		NameComponent(const NameComponent&) = default;
		NameComponent(const std::string& name)
			: name(name) {}
	};

	struct TransformComponent
	{
		vec3 position;
		quat rotation;
		vec3 scale;

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(vec3 position, quat rotation, vec3 scale)
			: position(position), rotation(rotation), scale(scale) {}
	};

	struct CustomScript
	{
		std::string name;
		std::vector<uint8_t> data;

		std::function<void()> onScriptCreateCallback;
		std::function<void()> onScriptDestroyCallback;
	};

	struct CustomScriptCompoent 
	{
		std::vector<CustomScript> scripts;

		CustomScriptCompoent() = default;
		CustomScriptCompoent(const CustomScriptCompoent&) = default;
		~CustomScriptCompoent()
		{
			for (auto& s : scripts)
			{
				if (s.onScriptDestroyCallback != nullptr)
				{
					s.onScriptDestroyCallback();
				}
			}
		}
	};

	struct RenderableComponent
	{
		// uuid of material in material manager
		// TODO: multiple materials aka. multiple render passes
		UUID		targetMaterial;
		// uuid of mesh in mesh pool
		UUID		targetMesh;
		// flags for some render options
		uint64_t	renderOptionFlags;

		RenderableComponent(const RenderableComponent&) = default;
		RenderableComponent() = default;
	};

	struct CameraComponent
	{
		float m_Far, m_Near, m_AspectRatio, m_Fov;

		CameraComponent() = default;
		CameraComponent(const CameraComponent& comp) = default;

		CameraComponent(float cameraFar, float cameraNear, float cameraAspect, float cameraFov)
			:m_Far(cameraFar), m_Near(cameraNear), m_AspectRatio(cameraAspect), m_Fov(cameraFov) 
		{}
	};


	template<typename ...Args>
	struct ComponentGroup {};
	using AllCopiableComponents = ComponentGroup<TransformComponent, RenderableComponent, CameraComponent, CustomScriptCompoent>;

}