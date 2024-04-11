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

	struct CameraFrustrum
	{
		vec4 plane[6];
	};

	class RenderCamera
	{
	public:
		RenderCamera() = default;
		RenderCamera(Entity e);
		RenderCamera(const RenderCamera& camera) = default;
		
		CameraUBO		GetCameraUBO();
		Transform&		GetCameraTransform();
		
		CameraFrustrum	GetFrustrum()
		{
			return GetFrustrum(0, 1, 0, 1, 0, 1);
		}
		CameraFrustrum  GetFrustrum(float u_tile, float u_tile_1, float v_tile, float v_tile_1, float d, float d_1);


	private:
		CameraComponent     m_Camera;
		Transform			m_Transform;
	};
}