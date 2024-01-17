#include "RenderCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Scene/Entity.h"

namespace kbs
{
	RenderCamera::RenderCamera(Entity e):
		m_Camera(e.GetComponent<CameraComponent>()),m_Transform(e.GetComponent<TransformComponent>(), e)
	{}

	CameraUBO kbs::RenderCamera::GetCameraUBO()
    {
		vec3 pos = m_Transform.GetPosition();
		glm::mat4 proj = glm::perspectiveLH_ZO( m_Camera.m_Fov, m_Camera.m_AspectRatio, m_Camera.m_Near, m_Camera.m_Far);
		glm::mat4 view = glm::lookAt(pos, pos + m_Transform.GetFront(), glm::vec3(0, 1, 0));

		proj[1][1] *= -1;

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

}


