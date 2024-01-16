#pragma once
#include "Scene/Components.h"

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
		Transform(TransformComponent trans);
		Transform(const Transform&) = default;

		vec3		GetPosition() const;
		vec3		GetFront() const;
		vec3		GetRight() const;
		quat		GetRotation() const;

		void		GoForward(float distance);
		void		GoRight(float distance);

		void		LookAt(vec3 pos);
		void		FaceDirection(vec3 dir);

		void		SetPosition(vec3 pos);
		void		SetRotation(quat q);

		ObjectUBO			GetObjectUBO();
		TransformComponent	GetComponent();
	private:
		TransformComponent m_Trans;
	};
}