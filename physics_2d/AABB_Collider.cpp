#include "AABB_Collider.hpp"

#include "BedrockAssert.hpp"
#include "BedrockDeffer.hpp"

namespace MFA
{

    //==================================================================================================================

    AABB_Collider::AABB_Collider(
        Transform* transform,
        Physics2D::Layer const layer,
        Physics2D::Layer const layerMask,
        glm::vec2 const& size,
        glm::vec2 const& offset,
        bool const isTrigger
    )
        : Collider2d(transform, layer, layerMask, isTrigger)
    {
        MFA_ASSERT(transform != nullptr);

        SetSize(size);
        SetOffset(offset);

        auto * physics = Physics2D::Instance;
        MFA_ASSERT(physics != nullptr);

        mEntityId = physics->Register(
            Physics2D::Type::AABB,
            mLayer,
            mLayerMask,
            nullptr  // TODO: Add callback if needed
        );
        UpdatePhysicsAABB(false);
        // Register for transform dirty notifications
        mTransformDirtySignalId = mTransform->DirtyListener.Register(this, &AABB_Collider::SetTransformDirty);
    }

    //==================================================================================================================

    AABB_Collider::~AABB_Collider()
    {
        // Unregister from transform dirty signal
        if (mTransform && mTransformDirtySignalId != SignalIdInvalid)
        {
            mTransform->DirtyListener.UnRegister(mTransformDirtySignalId);
        }
    }

    //==================================================================================================================

    void AABB_Collider::Update()
    {
        bool const isDirty = mIsSizeDirty | mIsOffsetDirty | mIsTransformDirty;
        if (isDirty == false)
        {
            return;
        }

        UpdatePhysicsAABB();
    }

    //==================================================================================================================

    AABB2D AABB_Collider::GetAABB(glm::vec2 position)
    {
        UpdateSize();
        UpdateOffset();

        position += mGlobalOffset;
        glm::vec2 halfSize = mGlobalSize * 0.5f;

        AABB2D aabb {
            .min = position - halfSize,
            .max = position + halfSize,
        };

        return aabb;
    }

    //==================================================================================================================

    void AABB_Collider::UpdatePhysicsAABB(bool checkForCollision)
    {
        MFA_ASSERT(mTransform != nullptr);
        MFA_ASSERT(Physics2D::Instance != nullptr);

        glm::vec3 finalPosition = mTransform->GlobalPosition();

        if (mIsTrigger == true || checkForCollision == false)
        {
            AABB2D const & aabb = GetAABB(finalPosition);
            bool success = Physics2D::Instance->MoveAABB(mEntityId, aabb.min, aabb.max, false);
            MFA_ASSERT(success == true);
            mStartPosition = finalPosition.xy;
        }
        else
        {
            static constexpr float epsilon = 1e-2f;

            glm::vec2 moveVector = mTransform->GlobalPosition().xy - mStartPosition;
            glm::vec2 remMoveVector = moveVector;

            AABB2D startAABB = GetAABB(mStartPosition);

            glm::vec2 startPoses[]
            {
                startAABB.min,
                startAABB.max,
                glm::vec2{startAABB.max.x, startAABB.min.y},
                glm::vec2{startAABB.min.x, startAABB.max.y}
            };

            constexpr int vCount = sizeof(startPoses) / sizeof(glm::vec2);
            MFA_ASSERT(vCount == 4);

            do
            {
                auto const moveMag = glm::length(remMoveVector);
                if (moveMag < epsilon)
                {
                    break;
                }

                glm::vec2 const moveDir = remMoveVector / moveMag;

                bool hit = false;
                float timeOfHit = 1.0f;
                glm::vec2 hitNormal{};

                for (int i = 0; i < vCount; ++i)
                {
                    Physics2D::HitInfo hitInfo{};
                    auto const localHit = Physics2D::Instance->Raycast(
                        mLayerMask,
                        {mEntityId},
                        Physics2D::Ray{startPoses[i], moveDir},
                        moveMag,
                        hitInfo
                    );

                    if (localHit == true)
                    {
                        if (hit == false || hitInfo.hitTime < timeOfHit)
                        {
                            timeOfHit = hitInfo.hitTime;
                            hitNormal = hitInfo.hitNormal;
                        }
                        hit = true;
                    }
                }

                glm::vec2 appliedMoveVector = timeOfHit * remMoveVector;
                if (hit == true)
                {
                    if (glm::length(appliedMoveVector) > epsilon)
                    {
                        appliedMoveVector -= epsilon * appliedMoveVector;
                    }
                    else
                    {
                        appliedMoveVector = {};
                    }
                }

                remMoveVector -= appliedMoveVector;
                if (hit == true)
                {
                    auto const dot = glm::dot(hitNormal, remMoveVector);
                    remMoveVector = remMoveVector - (dot * hitNormal);
                }

                mStartPosition = mStartPosition + appliedMoveVector;

                startAABB = GetAABB(mStartPosition);

                startPoses[0] = startAABB.min;
                startPoses[1] = startAABB.max;
                startPoses[2] = glm::vec2{startAABB.max.x, startAABB.min.y};
                startPoses[3] = glm::vec2{startAABB.min.x, startAABB.max.y};

                auto const success = Physics2D::Instance->MoveAABB(
                    mEntityId,
                    startAABB.min,
                    startAABB.max,
                    false
                );
                MFA_ASSERT(success == true);

                mTransform->SetLocalPosition(glm::vec3{mStartPosition.x, mStartPosition.y, finalPosition.z});
            }
            while (true);
        }
    }

    //==================================================================================================================

    void AABB_Collider::SetTransformDirty() {mIsTransformDirty = true;}

    //==================================================================================================================

    void AABB_Collider::SetSizeDirty() {mIsSizeDirty = true;}

    //==================================================================================================================

    void AABB_Collider::SetOffsetDirty() {mIsOffsetDirty = true;}

    //==================================================================================================================

    void AABB_Collider::UpdateSize()
    {
        if (mIsSizeDirty == true)
        {
            mIsSizeDirty = false;
            mGlobalSize = mTransform->GlobalTransform() * glm::vec4(mSize.xy, 0.0f, 0.0f);
        }
    }

    //==================================================================================================================

    void AABB_Collider::UpdateOffset()
    {
        if (mIsOffsetDirty == true)
        {
            mIsOffsetDirty = false;
            mGlobalOffset = mTransform->GlobalTransform() * glm::vec4(mOffset.xy, 0.0f, 0.0f);
        }
    }

    //==================================================================================================================

} // namespace MFA
