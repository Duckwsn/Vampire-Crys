#pragma once

#include "gameplay/Definitions.h"
#include "raylib.h"

struct Enemy;
struct Boss;
struct Projectile;

namespace VisualStyle {
constexpr int EnvironmentCoverageMargin = 160;
constexpr int ViewportFadeWidth = 48;

constexpr int EnvironmentHalfCoverage(int viewportExtent) {
    return viewportExtent / 2 + EnvironmentCoverageMargin;
}

Color EnergyColor(float time, float saturation = 0.68f, float value = 1.0f);
Color DarkPanel(unsigned char alpha = 235);

void DrawEnvironment(Vector2 center, Rectangle arena, float time);
void DrawViewportFade();
void DrawPlayer(Vector2 position, float radius, Vector2 facing, bool moving,
                bool flashing, float time, Color characterColor = WHITE, bool dead = false);
void DrawEnemy(const Enemy& enemy, float time);
void DrawBoss(const Boss& boss, float time);
void DrawProjectile(const Projectile& projectile, float time);
void DrawWeaponIcon(WeaponType type, EvolvedWeaponType evolved, Rectangle bounds, float time);
void DrawPassiveIcon(PassiveType type, Rectangle bounds, float time);
void DrawPickupIcon(int type, Vector2 position, float radius, float time);
void DrawChest(Vector2 position, float radius, float time);
}
