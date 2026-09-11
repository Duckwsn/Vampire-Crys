#pragma once

#include "raylib.h"

namespace Balance {
constexpr int ScreenWidth = 1280;
constexpr int ScreenHeight = 720;
constexpr Rectangle Arena{-2000.0f, -2000.0f, 4000.0f, 4000.0f};

constexpr float PlayerRadius = 18.0f;
constexpr float PlayerMaxHP = 100.0f;
constexpr float PlayerMoveSpeed = 235.0f;
constexpr float PlayerInvulnerability = 0.5f;
constexpr float PlayerMagnetRadius = 105.0f;

constexpr int EnemyPoolSize = 600;
constexpr int ProjectilePoolSize = 800;
constexpr int XPOrbPoolSize = 800;
constexpr float XPOrbRadius = 7.0f;

inline int XPRequiredForLevel(int level) {
    return 12 + (level - 1) * 8;
}
} // namespace Balance
