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


	template<typename ...Args>
	struct ComponentGroup {};
	using AllCopiableComponents = ComponentGroup<TransformComponent>;

}