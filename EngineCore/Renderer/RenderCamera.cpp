#include "RenderCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Scene/Entity.h"

#include <iostream>

namespace kbs
{
	RenderCamera::RenderCamera(Entity e):
		m_Camera(e.GetComponent<CameraComponent>()),m_Transform(e.GetComponent<TransformComponent>(), e)
	{}

	RenderCamera::RenderCamera(const CameraComponent& c, const Transform& trans) :
		m_Camera(c), m_Transform(trans)
	{}

	CameraUBO kbs::RenderCamera::GetCameraUBO()
    {
		vec3 pos = m_Transform.GetPosition();
		glm::mat4 proj;

		if (m_Camera.m_Type == CameraComponent::CameraType::Perspective)
		{
			proj = glm::perspectiveLH_ZO(m_Camera.m_Fov, m_Camera.m_AspectRatio, m_Camera.m_Near, m_Camera.m_Far);
			proj[1][1] *= -1;
		}
		else if(m_Camera.m_Type == CameraComponent::CameraType::Orthogonal)
		{
			proj = glm::orthoLH_ZO(-m_Camera.m_Width, m_Camera.m_Width, -m_Camera.m_Height, m_Camera.m_Height, m_Camera.m_Near, m_Camera.m_Far);
		}
		glm::mat4 view = glm::lookAt(pos, pos + m_Transform.GetFront(), glm::vec3(0, 1, 0));

		CameraUBO ubo;
		ubo.cameraPosition = pos;
		ubo.invProjection = glm::inverse(proj);
		ubo.projection = proj;
		ubo.invView = m_Transform.GetObjectUBO().model;
		ubo.view = glm::inverse(ubo.invView);
		return ubo;
    }

	Transform& RenderCamera::GetCameraTransform()
	{
		return m_Transform;
	}

	kbs::CameraFrustrum RenderCamera::GetFrustrum(float u_tile, float u_tile_1, float v_tile, float v_tile_1, float d, float d_1)
	{
		CameraUBO ubo = GetCameraUBO();

		mat4  m = ubo.projection * ubo.view;

		vec4 right = -vec4(u_tile_1 * m[0][3] - m[0][0], u_tile_1 * m[1][3] - m[1][0], u_tile_1 * m[2][3] - m[2][0], u_tile_1 * m[3][3] - m[3][0]);
		vec4 up = -vec4(v_tile_1 * m[0][3] - m[0][1], v_tile_1 * m[1][3] - m[1][1], v_tile_1 * m[2][3] - m[2][1], v_tile_1 * m[3][3] - m[3][1]);
		vec4 front = -vec4(d_1 * m[0][3] - m[0][2], d_1 * m[1][3] - m[1][2], d_1 * m[2][3] - m[2][2], d_1 * m[3][3] - m[3][2]);

		vec4 left = -vec4(m[0][0] - u_tile * m[0][3], m[1][0] - u_tile * m[1][3], m[2][0] - u_tile * m[2][3], m[3][0] - u_tile * m[3][3]);
		vec4 down = -vec4(m[0][1] - v_tile * m[0][3], m[1][1] - v_tile * m[1][3], m[2][1] - v_tile * m[2][3], m[3][1] - v_tile * m[3][3]);
		vec4 back = -vec4(m[0][2] - d * m[0][3], m[1][2] - d * m[1][3], m[2][2] - d * m[2][3], m[3][2] - d * m[3][3]);

		CameraFrustrum frustrum;
		frustrum.plane[0] = right;
		frustrum.plane[1] = left;
		frustrum.plane[2] = up;
		frustrum.plane[3] = down;
		frustrum.plane[4] = front;
		frustrum.plane[5] = back;

		return frustrum;
	}
	
}


