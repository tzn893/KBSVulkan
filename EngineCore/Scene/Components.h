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

		struct
		{
			bool  castShadow;
			float distance;
			float bias;
			float windowWidth;
			float windowHeight;
		} shadowCaster;
		
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
		enum CameraType
		{
			Perspective,
			Orthogonal
		} m_Type = Perspective;

		float m_Far = 0, m_Near = 0;
		
		float m_AspectRatio = 0;
		float m_Width = 0;
		
		float m_Fov = 0;
		float m_Height = 0;

		CameraComponent() = default;
		CameraComponent(const CameraComponent& comp)
		{
			m_Fov = comp.m_Fov;
			m_Height = comp.m_Height;
			m_Width = comp.m_Width;
			m_AspectRatio = comp.m_AspectRatio;

			m_Far = comp.m_Far;
			m_Near = comp.m_Near;
			m_Type = comp.m_Type;
		}

		CameraComponent(float cameraFar, float cameraNear, float cameraAspect, float cameraFov)
			:m_Far(cameraFar), m_Near(cameraNear), m_AspectRatio(cameraAspect), m_Fov(cameraFov),m_Type(CameraType::Perspective)
		{}

		static CameraComponent OrthogonalCamera(float cameraFar, float cameraNear, float width, float height)
		{
			CameraComponent cameraComponent;
			cameraComponent.m_Width = width / 2;
			cameraComponent.m_Height = height / 2;
			cameraComponent.m_Type = CameraType::Orthogonal;
			cameraComponent.m_Far = cameraFar;
			cameraComponent.m_Near = cameraNear;

			return cameraComponent;
		}

		static CameraComponent PerspectiveCamera(float cameraFar, float cameraNear, float aspect, float fov)
		{
			return CameraComponent(cameraFar, cameraNear, aspect, fov);
		}
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