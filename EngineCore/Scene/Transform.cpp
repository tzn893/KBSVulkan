#include "Transform.h"
#include "Scene/Entity.h"

namespace kbs
{
    kbs::Transform::Transform(TransformComponent trans,Entity entity)
        :m_Trans(trans), m_Entity(entity)
    {
    }


    vec3 kbs::Transform::GetPosition() 
    {
        vec3 position = m_Trans.position;
        TranverseParentTransform(
            [&](TransformComponent& parentTrans)
           {
                position = parentTrans.rotation * (parentTrans.scale * position) + parentTrans.position;
            }
        );
        return position;
    }

    vec3 Transform::GetLocalPosition() 
    {
        return m_Trans.position;
    }

    vec3 kbs::Transform::GetFront() 
    {
        vec3 front = GetLocalFront();
        TranverseParentTransform(
            [&](TransformComponent& parentTrans)
            {
                front = parentTrans.rotation * front;
            }
        );
        return front;
    }

    vec3 Transform::GetLocalFront() 
    {
        vec3 front = vec3(0, 0, 1);
        return m_Trans.rotation * front;
    }

    vec3 Transform::GetRight() 
    {
        vec3 right = GetLocalRight();
        TranverseParentTransform(
            [&](TransformComponent& parentTrans)
            {
                right = parentTrans.rotation * right;
            }
        );
        return right;
    }

    vec3 Transform::GetLocalRight() 
    {
        vec3 right = vec3(1, 0, 0);
        return m_Trans.rotation * right;
    }

    quat kbs::Transform::GetRotation() 
    {
        quat rot = GetLocalRotation();
        TranverseParentTransform(
            [&](TransformComponent& parentTrans)
            {
                rot = parentTrans.rotation * rot;
            }
        );
        return rot;
    }

    quat Transform::GetLocalRotation() 
    {
        return m_Trans.rotation;
    }


    void Transform::LookAt(vec3 pos)
    {
        vec3 dir = GetPosition() 
            - pos;
        if (math::length(dir) > 1e-4)
        {
            FaceDirection(math::normalize(dir));
        }
    }

    void Transform::FaceDirection(vec3 dir)
    {
        quat rotation = GetRotation();
        dir = glm::inverse(rotation)* dir;

        vec3  axis  = math::cross(vec3(0, 0, 1), dir);
        Angle angle = math::acos(math::dot(vec3(0, 0, 1), dir));
        m_Trans.rotation = math::axisAngle(axis, angle);
    }

    void kbs::Transform::SetPosition(vec3 pos)
    {
        mat4 model = math::position(vec3(0, 0, 0));
        TranverseParentTransform(
            [&](TransformComponent& comp)
            {
                model = math::position(comp.position) * math::quat2mat(m_Trans.rotation) * math::scale(m_Trans.scale) * model;
            }
        );
        vec4 newPos = glm::inverse(model) * vec4(pos,1);
        m_Trans.position = vec3(newPos.x, newPos.y, newPos.z);
    }

    void Transform::SetLocalPosition(vec3 pos)
    {
        m_Trans.position = pos;
    }

    void kbs::Transform::SetRotation(quat q)
    {
        quat worldQ = GetRotation();
        m_Trans.rotation = glm::inverse(worldQ) * q;
    }

    void Transform::SetLocalRotation(quat q)
    {
        m_Trans.rotation = q;
    }

    ObjectUBO kbs::Transform::GetObjectUBO()
    {
        mat4 model = math::position(m_Trans.position)* math::quat2mat(m_Trans.rotation)* math::scale(m_Trans.scale);
        TranverseParentTransform(
            [&](TransformComponent& comp)
            {
                model = math::position(comp.position) * math::quat2mat(comp.rotation) * math::scale(comp.scale) * model;
            }
        );
        mat4 invTransModel = math::transpose(math::inverse(model));

        return ObjectUBO{model, invTransModel};
    }

    TransformComponent kbs::Transform::GetComponent()
    {
        return m_Trans;
    }


    void Transform::TranverseParentTransform(std::function<void(TransformComponent&)> op)
    {
        Scene* scene = m_Entity.GetScene();
        UUID   currentComp = m_Trans.parent;

        while (currentComp != scene->GetRootID())
        {
            TransformComponent trans = scene->GetEntityByUUID(currentComp).GetComponent<TransformComponent>();
            op(trans);
            currentComp = trans.parent;
        }
    }
}


