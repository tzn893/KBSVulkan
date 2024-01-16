#pragma once
#include "Common.h"
#include "Scene/Scene.h"
#include "Scene/Transform.h"
// Camera used in rendering


namespace kbs
{
	struct CameraUBO
	{
		mat4 invProjection;
		mat4 projection;
		mat4 invView;
		mat4 view;
		vec3 cameraPosition;
	};

	class RenderCameraCullingResult
	{

	};

	class RenderCamera
	{
	public:
		RenderCamera() = default;
		RenderCamera(CameraComponent camera, TransformComponent transform);
		RenderCamera(const RenderCamera& camera) = default;
		
		CameraUBO	GetCameraUBO();
		Transform&	GetCameraTransform();
		static opt<RenderCamera> CreateRenderCamera(Entity entity);

	private:
		CameraComponent     m_Camera;
		Transform			m_Transform;
	};
}