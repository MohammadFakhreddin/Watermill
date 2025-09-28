#pragma once

#include "Physics2D.hpp"

namespace Constants
{
    namespace Layers
    {
        // Layer definitions
        static constexpr MFA::Physics2D::Layer Invalid = 0;
        static constexpr MFA::Physics2D::Layer Block = 1 << 0;
        static constexpr MFA::Physics2D::Layer Player = 1 << 1;
        static constexpr MFA::Physics2D::Layer Enemy = 1 << 2;
        static constexpr MFA::Physics2D::Layer Coin = 1 << 3;
        static constexpr MFA::Physics2D::Layer SpawnPoint = 1 << 4;
        static constexpr MFA::Physics2D::Layer FinishPoint = 1 << 4;
    }

    namespace Masks
    {
        // Layer masks define what each layer collides with
        // Everything collides with Player, but not with each other
        static constexpr MFA::Physics2D::Layer EmptyMask = 0;
        static constexpr MFA::Physics2D::Layer WallMask = Layers::Player;
        static constexpr MFA::Physics2D::Layer PlayerMask = Layers::Block | Layers::Enemy | Layers::Coin;
        static constexpr MFA::Physics2D::Layer EnemyMask = Layers::Player;
        static constexpr MFA::Physics2D::Layer CoinMask = Layers::Player;
        static constexpr MFA::Physics2D::Layer FinishMask = Layers::Player;
    }

    namespace GameTagKeys
    {
        static constexpr auto Block = "untagged";
        static constexpr auto Player = "player";
        static constexpr auto Enemy = "enemy";
        static constexpr auto Coin = "coin";
        static constexpr auto SpawnPoint = "spawn";
        static constexpr auto FinishPoint = "finish";
    }

    enum class GameTags
    {
        Invalid,
        Block,
        Player,
        Enemy,
        Coin,
        SpawnPoint,
        FinishPoint
    };

}