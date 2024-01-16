#include "Transform.h"

namespace kbs
{
    kbs::Transform::Transform(TransformComponent trans)
        :m_Trans(trans)
    {
    }

    vec3 kbs::Transform::GetPosition() const
    {
        return m_Trans.position;
    }

    vec3 kbs::Transform::GetFront() const
    {
        vec3 front = vec3(0, 0, 1);
        return m_Trans.rotation * front;
    }

    vec3 Transform::GetRight() const
    {
        vec3 right = vec3(1, 0, 0);
        return m_Trans.rotation * right;
    }

    quat kbs::Transform::GetRotation() const
    {
        return m_Trans.rotation;
    }

    void Transform::GoForward(float distance) 
    {
        m_Trans.position += GetFront() * distance;
    }

    void Transform::GoRight(float distance)
    {
        m_Trans.position += GetRight() * distance;
    }

    void Transform::LookAt(vec3 pos)
    {
        vec3 dir = m_Trans.position - pos;
        if (math::length(dir) > 1e-4)
        {
            FaceDirection(math::normalize(dir));
        }
    }

    void Transform::FaceDirection(vec3 dir)
    {
        vec3  axis  = math::cross(vec3(0, 0, 1), dir);
        Angle angle = math::acos(math::dot(vec3(0, 0, 1), dir));
        m_Trans.rotation = math::axisAngle(axis, angle);
    }

    void kbs::Transform::SetPosition(vec3 pos)
    {
        m_Trans.position = pos;
    }

    void kbs::Transform::SetRotation(quat q)
    {
        m_Trans.rotation = q;
    }

    ObjectUBO kbs::Transform::GetObjectUBO()
    {
        mat4 model = math::position(m_Trans.position)* math::quat2mat(m_Trans.rotation)* math::scale(m_Trans.scale);
        mat4 invTransModel = math::transpose(math::inverse(model));

        return ObjectUBO{model, invTransModel};
    }

    TransformComponent kbs::Transform::GetComponent()
    {
        return m_Trans;
    }
}


