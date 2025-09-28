#pragma once

#include "AABB_Collider.hpp"
#include "Transform.hpp"
#include <glm/vec2.hpp>

class Player
{
public:

    explicit Player(
        MFA::Transform* transform,
        MFA::AABB_Collider* collider,
        float moveSpeed = 5.0f
    );

    void Update(float deltaTime);

    void SetMovementInput(glm::vec2 const& input);

    [[nodiscard]]
    MFA::Transform* GetTransform() const { return mTransform; }

    [[nodiscard]]
    glm::vec2 GetPosition() const;

private:

    MFA::Transform* mTransform = nullptr;
    MFA::AABB_Collider* mCollider = nullptr;

    float mMoveSpeed = 5.0f;
    glm::vec2 mMovementInput{0.0f, 0.0f}; // x: left/right, y: up/down (future use)

    glm::vec3 mLastPosition{};
};