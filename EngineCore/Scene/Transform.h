#pragma once
#include "Scene/Components.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"

namespace kbs
{
	struct ObjectUBO
	{
		mat4 model;
		mat4 invTransModel;
	};

	class Transform
	{
	public:
		Transform() = default;
		Transform(TransformComponent trans, Entity entity);
		Transform(const Transform&) = default;

		vec3		GetPosition();
		vec3		GetLocalPosition();
		vec3		GetFront();
		vec3		GetLocalFront();
		vec3		GetRight();
		vec3		GetLocalRight();
		quat		GetRotation();
		quat		GetLocalRotation();

		void		LookAt(vec3 pos);
		void		FaceDirection(vec3 dir);

		void		SetPosition(vec3 pos);
		void		SetLocalPosition(vec3 pos);
		void		SetRotation(quat q);
		void		SetLocalRotation(quat q);

		ObjectUBO			GetObjectUBO();
		TransformComponent	GetComponent();
	private:
		void TranverseParentTransform(std::function<void(TransformComponent&)>);

		TransformComponent m_Trans;
		Entity			   m_Entity;
	};
}