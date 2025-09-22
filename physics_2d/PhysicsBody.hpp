#pragma once
#include <vec2.hpp>

#include "Physics2D.hpp"

namespace MFA
{
    // TODO: We need entity system + compoennts to query for stuff
    class PhysicsBody
    {
    public:

        struct Params
        {
            float moveSpeed = 5.0f;
            float rotationSpeed = 10.0f;
            glm::vec2 halfColliderExtent{0.5, 0.5};
            glm::vec2 gravity{};
        };

        explicit PhysicsBody(Physics2D::EntityID id, Params const & params);

        ~PhysicsBody();

        void Teleport(glm::vec2 const & pos2d);

        void Move(glm::vec2 const & vector);

    };
}
