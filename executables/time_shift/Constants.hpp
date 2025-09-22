#pragma once

#include "Physics2D.hpp"

namespace TimeShift
{
    namespace Layers
    {
        // Layer definitions
        static constexpr MFA::Physics2D::Layer Invalid = 0;
        static constexpr MFA::Physics2D::Layer Wall = 1 << 0;
        static constexpr MFA::Physics2D::Layer Player = 1 << 1;
        static constexpr MFA::Physics2D::Layer Enemy = 1 << 2;
        static constexpr MFA::Physics2D::Layer Lava = 1 << 3;

        // Layer masks define what each layer collides with
        // Everything collides with Player, but not with each other
        static constexpr MFA::Physics2D::Layer EmptyMask = 0;
        static constexpr MFA::Physics2D::Layer WallMask = Player;
        static constexpr MFA::Physics2D::Layer PlayerMask = Wall | Enemy | Lava;
        static constexpr MFA::Physics2D::Layer EnemyMask = Player;
        static constexpr MFA::Physics2D::Layer LavaMask = Player;
    }

    namespace Tags
    {
        static constexpr const char* Wall = "wall";
        static constexpr const char* Player = "player";
        static constexpr const char* Enemy = "enemy";
        static constexpr const char* Lava = "lava";
    }
}