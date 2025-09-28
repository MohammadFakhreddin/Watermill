#pragma once

#include "Physics2D.hpp"
#include "Transform.hpp"

namespace MFA
{
    class Collider2d
    {
    public:

        explicit Collider2d(
            Transform * transform,
            Physics2D::Layer layer,
            Physics2D::Layer layerMask,
            bool isTrigger = false
        );

        virtual ~Collider2d();

        virtual void Update() = 0;

    protected:

        Transform* mTransform = nullptr;
        Physics2D::EntityID mEntityId = 0;
        Physics2D::Layer mLayer = 0;
        Physics2D::Layer mLayerMask = 0;
        bool mIsTrigger = false;
        glm::vec2 mLastPosition{};
        bool mPositionDirty = true;
    };
}
