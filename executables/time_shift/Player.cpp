#include "Player.hpp"

#include "BedrockAssert.hpp"

Player::Player(
    MFA::Transform* transform,
    MFA::AABB_Collider* collider,
    float moveSpeed
)
    : mTransform(transform)
    , mCollider(collider)
    , mMoveSpeed(moveSpeed)
{
    MFA_ASSERT(transform != nullptr);
    MFA_ASSERT(collider != nullptr);

    mLastPosition = mTransform->GlobalPosition();
}

void Player::Update(float deltaTime)
{
    if (glm::length(mMovementInput) > 0.0f)
    {
        // Calculate movement vector
        glm::vec3 currentPosition = mTransform->GlobalPosition();
        glm::vec3 moveVector = glm::vec3(mMovementInput.xy, 0.0f) * mMoveSpeed * deltaTime;

        // Apply movement to transform
        glm::vec3 newPosition = currentPosition + moveVector;
        mTransform->SetGlobalPosition(newPosition);

        // The AABB_Collider will automatically handle collision detection
        // via the Transform signal system and update physics accordingly
    }
}

void Player::SetMovementInput(glm::vec2 const& input)
{
    mMovementInput = input;
}

glm::vec2 Player::GetPosition() const
{
    auto pos = mTransform->GlobalPosition();
    return glm::vec2(pos.x, pos.y);
}