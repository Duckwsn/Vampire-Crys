#include "ui/VisualStyle.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "systems/BossManager.h"
#include "systems/EnemyManager.h"
#include "systems/ProjectileManager.h"

namespace {
std::uint32_t Hash(int x, int y) {
    std::uint32_t value = static_cast<std::uint32_t>(x) * 0x9E3779B9u;
    value ^= static_cast<std::uint32_t>(y) + 0x85EBCA6Bu + (value << 6u) + (value >> 2u);
    value ^= value >> 16u;
    value *= 0x7FEB352Du;
    return value ^ (value >> 15u);
}

void DrawDiamond(Vector2 position, float radius, Color color, float rotation = 45.0f) {
    DrawPoly(position, 4, radius, rotation, color);
}

Vector2 Center(Rectangle bounds) {
    return {bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f};
}
}

Color VisualStyle::EnergyColor(float time, float saturation, float value) {
    const float hue = std::fmod(time * 48.0f, 360.0f);
    return ColorFromHSV(hue, saturation, value);
}

Color VisualStyle::DarkPanel(unsigned char alpha) { return Color{11, 13, 25, alpha}; }

void VisualStyle::DrawEnvironment(Vector2 center, Rectangle arena, float time) {
    constexpr int tile = 80;
    const int halfWidth = EnvironmentHalfCoverage(GetScreenWidth());
    const int halfHeight = EnvironmentHalfCoverage(GetScreenHeight());
    const int minX = std::max(static_cast<int>(arena.x), static_cast<int>(center.x) - halfWidth);
    const int maxX = std::min(static_cast<int>(arena.x + arena.width), static_cast<int>(center.x) + halfWidth);
    const int minY = std::max(static_cast<int>(arena.y), static_cast<int>(center.y) - halfHeight);
    const int maxY = std::min(static_cast<int>(arena.y + arena.height), static_cast<int>(center.y) + halfHeight);
    const int startX = static_cast<int>(std::floor(static_cast<float>(minX) / tile)) * tile;
    const int startY = static_cast<int>(std::floor(static_cast<float>(minY) / tile)) * tile;
    DrawRectangleRec(arena, Color{13, 16, 27, 255});
    for (int y = startY; y <= maxY; y += tile) {
        for (int x = startX; x <= maxX; x += tile) {
            const std::uint32_t seed = Hash(x / tile, y / tile);
            const Color ground = (seed & 3u) == 0u ? Color{20, 24, 37, 255}
                                                   : Color{17, 21, 33, 255};
            DrawRectangle(x, y, tile, tile, ground);
            DrawRectangleLines(x, y, tile, tile, Color{27, 31, 45, 90});
            const Vector2 prop{static_cast<float>(x + 12 + seed % 55u),
                               static_cast<float>(y + 12 + (seed >> 8u) % 55u)};
            switch ((seed >> 16u) % 11u) {
                case 0:
                    DrawLineEx({prop.x - 10, prop.y - 5}, {prop.x + 9, prop.y + 6}, 2.0f,
                               Color{48, 48, 61, 180});
                    DrawLineEx({prop.x - 2, prop.y - 10}, {prop.x + 4, prop.y + 10}, 2.0f,
                               Color{48, 48, 61, 180});
                    break;
                case 1:
                    DrawPoly(prop, 5, 7.0f, static_cast<float>(seed % 72u), Color{38, 43, 52, 210});
                    break;
                case 2:
                    DrawLineEx({prop.x, prop.y - 8}, {prop.x, prop.y + 8}, 2.0f, Color{43, 58, 47, 190});
                    DrawLineEx({prop.x, prop.y}, {prop.x + 6, prop.y - 5}, 2.0f, Color{43, 58, 47, 190});
                    break;
                default: break;
            }
        }
    }
    // Blend the finite arena into the same blue-black used by the outer composition.
    // The former solid outline made the world end at an aggressive rectangular line.
    constexpr float boundaryFade = 64.0f;
    const Color outer{13, 15, 20, 210};
    const Color inner{13, 15, 20, 0};
    DrawRectangleGradientH(static_cast<int>(arena.x), static_cast<int>(arena.y),
                           static_cast<int>(boundaryFade), static_cast<int>(arena.height), outer, inner);
    DrawRectangleGradientH(static_cast<int>(arena.x + arena.width - boundaryFade),
                           static_cast<int>(arena.y), static_cast<int>(boundaryFade),
                           static_cast<int>(arena.height), inner, outer);
    DrawRectangleGradientV(static_cast<int>(arena.x), static_cast<int>(arena.y),
                           static_cast<int>(arena.width), static_cast<int>(boundaryFade), outer, inner);
    DrawRectangleGradientV(static_cast<int>(arena.x),
                           static_cast<int>(arena.y + arena.height - boundaryFade),
                           static_cast<int>(arena.width), static_cast<int>(boundaryFade), inner, outer);
    (void)time;
}

void VisualStyle::DrawViewportFade() {
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();
    const Color edge{13, 15, 20, 92};
    const Color clear{13, 15, 20, 0};

    DrawRectangleGradientH(0, 0, ViewportFadeWidth, height, edge, clear);
    DrawRectangleGradientH(width - ViewportFadeWidth, 0, ViewportFadeWidth, height, clear, edge);
    DrawRectangleGradientV(0, 0, width, ViewportFadeWidth, edge, clear);
    DrawRectangleGradientV(0, height - ViewportFadeWidth, width, ViewportFadeWidth, clear, edge);
}

void VisualStyle::DrawPlayer(Vector2 position, float radius, Vector2 facing, bool moving,
                             bool flashing, float time, Color characterColor, bool dead) {
    const Color cycle = EnergyColor(time, 0.62f, 1.0f);
    const Color energy = flashing ? Color{255, 245, 245, 255} :
        Color{static_cast<unsigned char>((static_cast<int>(cycle.r) + characterColor.r) / 2),
              static_cast<unsigned char>((static_cast<int>(cycle.g) + characterColor.g) / 2),
              static_cast<unsigned char>((static_cast<int>(cycle.b) + characterColor.b) / 2), 255};
    const float breathe = 1.0f + std::sin(time * (moving ? 10.0f : 4.0f)) * 0.06f;
    const float side = radius * 1.62f * breathe;
    if (dead) {
        const Color faded{characterColor.r, characterColor.g, characterColor.b, 105};
        DrawCircleV(position, radius * 1.5f, Color{faded.r, faded.g, faded.b, 18});
        DrawRectanglePro({position.x, position.y, side * 1.15f, side * 0.28f},
                         {side * 0.575f, side * 0.14f}, 12.0f, faded);
        DrawLineEx({position.x - radius * 0.7f, position.y - radius * 0.7f},
                   {position.x + radius * 0.7f, position.y + radius * 0.7f}, 2.0f, Color{255, 235, 240, 150});
        DrawLineEx({position.x + radius * 0.7f, position.y - radius * 0.7f},
                   {position.x - radius * 0.7f, position.y + radius * 0.7f}, 2.0f, Color{255, 235, 240, 150});
        return;
    }
    DrawCircleV(position, radius * 1.62f, Color{energy.r, energy.g, energy.b, 26});
    DrawRectanglePro({position.x, position.y, side, side}, {side * 0.5f, side * 0.5f},
                     time * 18.0f, Color{energy.r, energy.g, energy.b, 65});
    DrawRectanglePro({position.x, position.y, radius * 1.48f, radius * 1.48f},
                     {radius * 0.74f, radius * 0.74f}, 0.0f, energy);
    DrawRectangleLinesEx({position.x - radius * 0.78f, position.y - radius * 0.78f,
                          radius * 1.56f, radius * 1.56f}, 2.5f, RAYWHITE);
    const Vector2 normalized = (facing.x == 0.0f && facing.y == 0.0f) ? Vector2{0.0f, 1.0f} : facing;
    DrawDiamond({position.x + normalized.x * radius * 0.48f,
                 position.y + normalized.y * radius * 0.48f}, 4.5f, WHITE);
}

void VisualStyle::DrawEnemy(const Enemy& enemy, float time) {
    const EnemyDefinition& definition = GetEnemyDefinition(enemy.type);
    Color color = enemy.hitFlash > 0.0f ? WHITE : definition.color;
    const float pulse = 1.0f + std::sin(time * 5.0f + enemy.position.x * 0.03f) * 0.06f;
    if (enemy.elite) {
        DrawCircleV(enemy.position, enemy.radius + 11.0f, Color{255, 205, 65, 32});
        DrawRing(enemy.position, enemy.radius + 5.0f, enemy.radius + 8.0f,
                 time * 75.0f, time * 75.0f + 245.0f, 28, Color{255, 211, 75, 210});
    }
    if (enemy.variant != EnemyVariant::None) {
        const Color variantColor = enemy.variant == EnemyVariant::Frenzied ? ORANGE :
                                   (enemy.variant == EnemyVariant::Armored ? SKYBLUE : VIOLET);
        DrawRing(enemy.position, enemy.radius + 2.0f, enemy.radius + 4.0f,
                 time * 55.0f, time * 55.0f + 170.0f, 18, variantColor);
    }
    if (enemy.slowRemaining > 0.0f)
        DrawRing(enemy.position, enemy.radius + 1.0f, enemy.radius + 3.0f,
                 0.0f, 360.0f, 18, Color{110, 220, 255, 170});
    if (enemy.intangible) color.a = 78;
    if (enemy.telegraphing) color = static_cast<int>(enemy.stateTimer * 16.0f) % 2 == 0 ? YELLOW : RED;

    if (enemy.type == EnemyType::Ghoul) {
        DrawDiamond(enemy.position, enemy.radius * pulse, color);
        DrawCircleV({enemy.position.x + 5, enemy.position.y - 3}, 3.0f, Color{25, 30, 22, 255});
    } else if (enemy.type == EnemyType::Swarmer) {
        DrawPoly(enemy.position, 3, enemy.radius * 1.25f * pulse, time * 90.0f, color);
        DrawLineEx({enemy.position.x - enemy.radius, enemy.position.y},
                   {enemy.position.x + enemy.radius, enemy.position.y}, 2.0f, RAYWHITE);
    } else if (enemy.type == EnemyType::Brute) {
        DrawPoly(enemy.position, 6, enemy.radius * pulse, 30.0f, color);
        DrawPolyLinesEx(enemy.position, 6, enemy.radius * 0.67f, 30.0f, 4.0f, Color{69, 36, 42, 255});
        DrawRectangleRec({enemy.position.x - 10, enemy.position.y - 5, 20, 10}, Color{54, 31, 36, 255});
    } else if (enemy.type == EnemyType::Cultist) {
        DrawPoly(enemy.position, 3, enemy.radius * 1.25f * pulse, -90.0f, color);
        DrawCircleV({enemy.position.x, enemy.position.y - 3}, 5.0f, Color{40, 20, 52, 255});
        DrawCircleLines(static_cast<int>(enemy.position.x), static_cast<int>(enemy.position.y - 3),
                        7.0f + std::sin(time * 7.0f) * 2.0f, VIOLET);
    } else if (enemy.type == EnemyType::Bomber) {
        DrawPoly(enemy.position, 8, enemy.radius * pulse, 22.5f, color);
        DrawDiamond(enemy.position, enemy.radius * 0.58f, YELLOW, time * 65.0f);
    } else if (enemy.type == EnemyType::ShieldedAcolyte) {
        DrawPoly(enemy.position, 6, enemy.radius * pulse, 30.0f, color);
        if (enemy.shieldHP > 0.0f)
            DrawRing(enemy.position, enemy.radius + 5.0f, enemy.radius + 9.0f,
                     time * 45.0f, time * 45.0f + 300.0f, 28, SKYBLUE);
    } else if (enemy.type == EnemyType::Wraith) {
        DrawPoly(enemy.position, 4, enemy.radius * pulse, time * 42.0f, color);
        DrawRing(enemy.position, enemy.radius * 0.35f, enemy.radius * 0.55f,
                 0, 360, 18, Color{230, 220, 255, color.a});
    } else if (enemy.type == EnemyType::Necromancer) {
        DrawPoly(enemy.position, 5, enemy.radius * pulse, -90.0f, color);
        DrawCircleV(enemy.position, 7.0f, BLACK);
        DrawRing(enemy.position, enemy.radius + 4.0f, enemy.radius + 7.0f,
                 time * -35.0f, time * -35.0f + 220.0f, 22, LIME);
    } else if (enemy.type == EnemyType::BoneMinion) {
        DrawPoly(enemy.position, 4, enemy.radius * pulse, 45.0f, color);
        DrawLineEx({enemy.position.x - 6, enemy.position.y},
                   {enemy.position.x + 6, enemy.position.y}, 2.0f, BROWN);
    } else if (enemy.type == EnemyType::GraveWarden) {
        DrawPoly(enemy.position, 8, enemy.radius * pulse, 22.5f, color);
        DrawPolyLinesEx(enemy.position, 4, enemy.radius * 0.68f, 45.0f, 6.0f, GOLD);
        DrawCircleV(enemy.position, 9.0f, BLACK);
    } else if (enemy.type == EnemyType::VoidStalker) {
        DrawPoly(enemy.position, 3, enemy.radius * 1.2f * pulse,
                 std::atan2(enemy.actionDirection.y, enemy.actionDirection.x) * RAD2DEG + 90.0f, color);
        DrawDiamond(enemy.position, 10.0f, WHITE, time * 90.0f);
    }
    DrawCircleLines(static_cast<int>(enemy.position.x), static_cast<int>(enemy.position.y),
                    enemy.radius, Color{25, 24, 35, 230});
}

void VisualStyle::DrawBoss(const Boss& boss, float time) {
    const bool flame = boss.type == BossType::FlameWyrm;
    const Color base = flame ? Color{244, 73, 28, 255} :
                       (boss.type == BossType::VoidHerald ? Color{116, 61, 190, 255}
                                                          : EnergyColor(time * 0.55f, 0.72f, 1.0f));
    const Color body = boss.hitFlash > 0.0f ? WHITE : base;
    const float pulse = 1.0f + std::sin(time * 4.0f) * 0.05f;
    DrawCircleV(boss.position, boss.radius + 22.0f, Color{base.r, base.g, base.b, 32});
    DrawRing(boss.position, boss.radius + 9.0f, boss.radius + 13.0f,
             time * 45.0f, time * 45.0f + (boss.phase == 2 ? 300.0f : 210.0f), 32,
             Color{base.r, base.g, base.b, 180});
    if (flame) {
        const float heading = std::atan2(boss.direction.y, boss.direction.x);
        for (int segment = 3; segment >= 0; --segment) {
            const float distance = boss.radius * (0.35f + segment * 0.38f);
            const Vector2 position{boss.position.x - std::cos(heading) * distance,
                                   boss.position.y - std::sin(heading) * distance};
            DrawDiamond(position, boss.radius * (0.78f - segment * 0.10f) * pulse,
                        segment == 0 ? body : Color{205, 57, 24, 255}, heading * RAD2DEG + 45.0f);
        }
        DrawPoly({boss.position.x + boss.direction.x * boss.radius * 0.55f,
                  boss.position.y + boss.direction.y * boss.radius * 0.55f},
                 3, 15.0f, heading * RAD2DEG + 90.0f, YELLOW);
    } else {
        DrawPoly(boss.position, boss.type == BossType::VoidHeraldAscended ? 8 : 6,
                 boss.radius * pulse, time * 12.0f, body);
        DrawPolyLinesEx(boss.position, 3, boss.radius * 0.72f, time * -24.0f,
                        4.0f, Color{225, 190, 255, 230});
        DrawCircleV(boss.position, 11.0f + std::sin(time * 8.0f) * 3.0f, BLACK);
        DrawDiamond(boss.position, 7.0f, RAYWHITE, time * 55.0f);
    }
}

void VisualStyle::DrawProjectile(const Projectile& projectile, float time) {
    const float speedAngle = std::atan2(projectile.velocity.y, projectile.velocity.x) * RAD2DEG;
    const Color glow{projectile.color.r, projectile.color.g, projectile.color.b,
                     static_cast<unsigned char>(projectile.owner == ProjectileOwner::Player ? 70 : 110)};
    if (projectile.owner == ProjectileOwner::Boss) {
        DrawPoly(projectile.position, 6, projectile.radius + 6.0f, time * 110.0f, glow);
        DrawDiamond(projectile.position, projectile.radius + 1.0f, projectile.color, time * -90.0f);
    } else if (projectile.owner == ProjectileOwner::Enemy) {
        DrawDiamond(projectile.position, projectile.radius + 5.0f, glow, time * 90.0f);
        DrawPoly(projectile.position, 3, projectile.radius + 1.0f, speedAngle + 90.0f, projectile.color);
        DrawCircleLines(static_cast<int>(projectile.position.x), static_cast<int>(projectile.position.y),
                        projectile.radius + 3.0f, RED);
    } else if (projectile.sourceWeapon == WeaponType::VoidLance) {
        const Vector2 direction{std::cos(speedAngle * DEG2RAD), std::sin(speedAngle * DEG2RAD)};
        DrawLineEx({projectile.position.x - direction.x * projectile.radius * 2.0f,
                    projectile.position.y - direction.y * projectile.radius * 2.0f},
                   {projectile.position.x + direction.x * projectile.radius * 2.0f,
                    projectile.position.y + direction.y * projectile.radius * 2.0f},
                   projectile.radius * 1.35f, glow);
        DrawDiamond(projectile.position, projectile.radius, projectile.color, speedAngle + 45.0f);
    } else if (projectile.sourceWeapon == WeaponType::ThunderCannon) {
        DrawCircleV(projectile.position, projectile.radius + 7.0f, glow);
        DrawPoly(projectile.position, 6, projectile.radius + 2.0f, time * 90.0f, projectile.color);
        DrawDiamond(projectile.position, projectile.radius * 0.55f, WHITE, time * -120.0f);
    } else if (projectile.sourceWeapon == WeaponType::FrostShards) {
        const Vector2 direction{std::cos(speedAngle * DEG2RAD), std::sin(speedAngle * DEG2RAD)};
        const Vector2 tail{projectile.position.x - direction.x * (projectile.radius + 9.0f),
                           projectile.position.y - direction.y * (projectile.radius + 9.0f)};
        DrawLineEx(tail, projectile.position, 3.0f, glow);
        DrawPoly(projectile.position, 4, projectile.radius + 2.0f,
                 speedAngle + 45.0f, projectile.color);
        DrawPolyLinesEx(projectile.position, 4, projectile.radius * 0.55f,
                        speedAngle + 45.0f, 1.5f, WHITE);
    } else if (projectile.sourceWeapon == WeaponType::BloodNeedles) {
        const Vector2 direction{std::cos(speedAngle * DEG2RAD), std::sin(speedAngle * DEG2RAD)};
        DrawLineEx({projectile.position.x - direction.x * 8.0f,
                    projectile.position.y - direction.y * 8.0f},
                   {projectile.position.x + direction.x * 7.0f,
                    projectile.position.y + direction.y * 7.0f},
                   2.4f, projectile.color);
        DrawCircleV(projectile.position, 2.0f, RAYWHITE);
    } else {
        DrawCircleV(projectile.position, projectile.radius + 4.0f, glow);
        DrawDiamond(projectile.position, projectile.radius + 1.0f, projectile.color, speedAngle + 45.0f);
    }
}

void VisualStyle::DrawWeaponIcon(WeaponType type, EvolvedWeaponType evolved, Rectangle bounds, float time) {
    const Vector2 center = Center(bounds);
    const float radius = std::min(bounds.width, bounds.height) * 0.31f;
    const Color base = evolved == EvolvedWeaponType::None ? GetWeaponDefinition(type).color
                                                          : EnergyColor(time + static_cast<int>(type), 0.6f, 1.0f);
    DrawRectangleRounded(bounds, 0.22f, 6, Color{19, 23, 38, 245});
    DrawRectangleRoundedLinesEx(bounds, 0.22f, 6, evolved == EvolvedWeaponType::None ? 2.0f : 3.0f, base);
    const int sides = 3 + static_cast<int>(type) % 5;
    DrawPoly(center, sides, radius, time * (evolved == EvolvedWeaponType::None ? 18.0f : 48.0f), base);
    DrawPolyLinesEx(center, sides, radius * 0.62f, -time * 32.0f, 2.0f, WHITE);
}

void VisualStyle::DrawPassiveIcon(PassiveType type, Rectangle bounds, float time) {
    const Vector2 center = Center(bounds);
    const Color color = GetPassiveDefinition(type).color;
    DrawRectangleRounded(bounds, 0.22f, 6, Color{19, 23, 38, 245});
    DrawRectangleRoundedLinesEx(bounds, 0.22f, 6, 2.0f, color);
    DrawRing(center, bounds.width * 0.16f, bounds.width * 0.27f, time * 35.0f,
             time * 35.0f + 250.0f, 18, color);
    DrawPoly(center, 4 + static_cast<int>(type) % 4, bounds.width * 0.15f,
             45.0f + time * 20.0f, RAYWHITE);
}

void VisualStyle::DrawPickupIcon(int type, Vector2 position, float radius, float time) {
    const Color color = type == 0 ? Color{255, 72, 105, 255} :
                        (type == 1 ? Color{70, 225, 255, 255} : Color{255, 208, 55, 255});
    DrawRing(position, radius + 5.0f, radius + 8.0f, time * 70.0f,
             time * 70.0f + 230.0f, 18, Color{color.r, color.g, color.b, 160});
    DrawPoly(position, type == 0 ? 4 : (type == 1 ? 6 : 8), radius,
             45.0f + time * 35.0f, color);
    if (type == 0) {
        DrawRectangle(static_cast<int>(position.x - 2), static_cast<int>(position.y - 8), 4, 16, WHITE);
        DrawRectangle(static_cast<int>(position.x - 8), static_cast<int>(position.y - 2), 16, 4, WHITE);
    } else if (type == 1) {
        DrawRing(position, 3.0f, 6.0f, 0.0f, 360.0f, 12, WHITE);
    } else {
        DrawDiamond(position, 6.0f, WHITE, time * -70.0f);
    }
}

void VisualStyle::DrawChest(Vector2 position, float radius, float time) {
    const float hover = std::sin(time * 3.2f) * 3.0f;
    position.y += hover;
    DrawCircleV(position, radius + 18.0f, Color{255, 205, 66, 30});
    DrawRing(position, radius + 7.0f, radius + 10.0f, time * 50.0f,
             time * 50.0f + 260.0f, 20, Color{255, 220, 90, 190});
    DrawPoly(position, 6, radius, 30.0f, Color{126, 69, 45, 255});
    DrawRectangleRec({position.x - radius, position.y - 3, radius * 2.0f, 7.0f}, GOLD);
    DrawDiamond(position, 7.0f, YELLOW, time * 40.0f);
}
