#pragma once

#include "Collider2d.hpp"
#include "Physics2D.hpp"
#include "Transform.hpp"
#include "AABB2D.hpp"
#include "BedrockCommon.hpp"
#include "BedrockSignalTypes.hpp"
#include <glm/vec2.hpp>

namespace MFA
{
    class AABB_Collider : public Collider2d
    {
    public:

        explicit AABB_Collider(
            Transform* transform,
            Physics2D::Layer layer,
            Physics2D::Layer layerMask,
            glm::vec2 const& size = glm::vec2(1.0f, 1.0f),
            glm::vec2 const& offset = glm::vec2(0.0f, 0.0f),
            bool isTrigger = false
        );

        ~AABB_Collider() override;

        void Update() override;

        [[nodiscard]]
        AABB2D GetAABB(glm::vec2 position);

    private:

        void UpdatePhysicsAABB(bool checkForCollision = true);

        void SetTransformDirty();
        void SetSizeDirty();
        void SetOffsetDirty();
        void UpdateSize();
        void UpdateOffset();

        MFA_VARIABLE3(Size, glm::vec2, glm::vec2(1.0f, 1.0f), SetSizeDirty, UpdateSize, m)
        MFA_VARIABLE3(Offset, glm::vec2, glm::vec2(0.0f, 0.0f), SetOffsetDirty, UpdateOffset, m)

        // We can have containers for variables that calculate them when dirty
        glm::vec2 mGlobalSize {};
        glm::vec2 mGlobalOffset {};
        glm::vec2 mStartPosition {};

        bool mIsSizeDirty = false;
        bool mIsOffsetDirty = false;
        bool mIsTransformDirty = false;

        AABB2D mAABB{};

        SignalId mTransformDirtySignalId = SignalIdInvalid;
    };
}