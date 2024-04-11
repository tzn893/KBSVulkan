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
		UUID parent;
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

	struct LightComponent
	{
		enum LightType
		{
			Area = 0,
			Sphere = 1,
			Directional = 2,
			ConstantEnvoriment = 3
			// Point = 3
		} type;

		vec3 intensity;
		union 
		{
			struct 
			{
				float radius;
			} sphere;

			struct 
			{
				vec3 u;
				vec3 v;
				float area;
			} area;
		};
		
		LightComponent() = default;
		LightComponent(const LightComponent&) = default;
	};

	struct RenderableComponent
	{
		constexpr static uint32_t maxPassCount = 8;
		uint64_t	renderOptionFlags[maxPassCount];
		// uuid of material in material manager
		// TODO: multiple materials aka. multiple render passes
		UUID		targetMaterials[maxPassCount];
		// uuid of mesh in mesh pool
		UUID		targetMesh;
		// flags for some render options
		uint32_t	passCount;


		void AddRenderablePass(UUID targetMaterial, uint64_t renderOptionFlag);
		void RemoveRenderablePass(UUID targetMaterial);


		RenderableComponent(const RenderableComponent&) = default;
		RenderableComponent()
		{
			passCount = 0;
			targetMesh = UUID::Invalid();
			for (uint32_t i = 0;i < maxPassCount;i++)
			{
				renderOptionFlags[i] = 0;
				targetMaterials[i] = UUID::Invalid();
			}
		}
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

	
	struct RayTracingGeometryComponent
	{
		RayTracingGeometryComponent() = default;
		RayTracingGeometryComponent(const RayTracingGeometryComponent&) = default;

		bool opaque;
		UUID rayTracingMaterial;
	};
	
	template<typename ...Args>
	struct ComponentGroup {};
	using AllCopiableComponents = ComponentGroup<TransformComponent, RenderableComponent, CameraComponent, CustomScriptCompoent, RayTracingGeometryComponent,
		LightComponent>;

}