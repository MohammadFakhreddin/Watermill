#pragma once

#include "Collider2d.hpp"
#include "Physics2D.hpp"
#include "Transform.hpp"

namespace MFA
{
    // Should extend Collider2d
    class AABB_Collider : public Collider2d
    {
    public:

        explicit AABB_Collider(
            Transform * transform,
            Physics2D layer,
            Physics2D layerMask,
            bool isTrigger = false
        );

        ~AABB_Collider();

    private:

        // MFA_VARIABLE3()

    };
}