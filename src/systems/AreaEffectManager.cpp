#include "systems/AreaEffectManager.h"

#include <algorithm>
#include <cmath>

#include "systems/EnemyManager.h"
#include "systems/BossManager.h"

AreaEffectManager::AreaEffectManager() {
    effects_.reserve(96);
    effects_.resize(96);
    queryScratch_.reserve(256);
}

void AreaEffectManager::Reset() {
    for (AreaEffect& effect : effects_) effect.active = false;
    activeCount_ = 0;
    poolWarningEmitted_ = false;
}

bool AreaEffectManager::Spawn(const AreaEffect& specification) {
    for (AreaEffect& effect : effects_) {
        if (effect.active) continue;
        effect = specification;
        effect.active = true;
        ++activeCount_;
        return true;
    }
    if (!poolWarningEmitted_) {
        TraceLog(LOG_WARNING, "Area effect pool full; effect skipped");
        poolWarningEmitted_ = true;
    }
    return false;
}

void AreaEffectManager::Update(float deltaTime, Vector2 playerPosition, EnemyManager& enemies,
                               BossManager* bosses) {
    for (AreaEffect& effect : effects_) {
        if (!effect.active) continue;
        if (effect.followPlayer) effect.position = playerPosition;
        effect.lifetime -= deltaTime;
        effect.tickTimer -= deltaTime;

        if (effect.tickTimer <= 0.0f && effect.ticksRemaining > 0) {
            auto& items = enemies.Items();
            enemies.QueryCircle(effect.position, effect.radius + 40.0f, queryScratch_);
            for (int rawIndex : queryScratch_) {
                const std::size_t index = static_cast<std::size_t>(rawIndex);
                Enemy& enemy = items[index];
                if (!enemy.active) continue;
                const float dx = enemy.position.x - effect.position.x;
                const float dy = enemy.position.y - effect.position.y;
                const float reach = effect.radius + enemy.radius;
                if (dx * dx + dy * dy > reach * reach) continue;
                const float length = std::sqrt(std::max(0.001f, dx * dx + dy * dy));
                if (effect.pullStrength > 0.0f) enemies.ApplyPull(index, effect.position, effect.pullStrength);
                if (effect.slowDuration > 0.0f) enemies.ApplySlow(index, effect.slowDuration, effect.slowMultiplier);
                enemies.Damage(index, effect.damage, {dx / length, dy / length},
                               effect.knockback, effect.source, effect.critical);
            }
            if (bosses != nullptr && bosses->IsActive()) {
                const Boss& boss = bosses->ActiveBoss();
                const float dx = boss.position.x - effect.position.x;
                const float dy = boss.position.y - effect.position.y;
                const float reach = effect.radius + boss.radius;
                if (dx * dx + dy * dy <= reach * reach) {
                    if (effect.slowDuration > 0.0f)
                        bosses->ApplySlow(effect.slowDuration, effect.slowMultiplier);
                    bosses->Damage(effect.damage, effect.source, effect.critical);
                }
            }
            --effect.ticksRemaining;
            effect.tickTimer += effect.tickInterval;
        }

        if (effect.lifetime <= 0.0f) {
            effect.active = false;
            --activeCount_;
        }
    }
}

void AreaEffectManager::Draw() const {
    for (const AreaEffect& effect : effects_) {
        if (!effect.active) continue;
        const float lifeFraction = effect.totalLifetime > 0.0f
                                       ? std::max(0.0f, effect.lifetime / effect.totalLifetime)
                                       : 0.0f;
        const Color fill{effect.color.r, effect.color.g, effect.color.b,
                         static_cast<unsigned char>(35 + lifeFraction * 45.0f)};
        const Color edge{effect.color.r, effect.color.g, effect.color.b,
                         static_cast<unsigned char>(120 + lifeFraction * 120.0f)};
        DrawCircleV(effect.position, effect.radius, fill);
        DrawCircleLines(static_cast<int>(effect.position.x), static_cast<int>(effect.position.y),
                        effect.radius, edge);
        if (effect.sourceWeapon == WeaponType::GravityWell) {
            const float phase = 1.0f - lifeFraction;
            for (int ring = 0; ring < 3; ++ring) {
                const float wrapped = std::fmod(phase * 1.8f + ring / 3.0f, 1.0f);
                DrawRing(effect.position, effect.radius * wrapped,
                         effect.radius * wrapped + 2.5f, 0.0f, 360.0f, 32,
                         Color{edge.r, edge.g, edge.b,
                               static_cast<unsigned char>(80 + 100 * (1.0f - wrapped))});
            }
            DrawPoly(effect.position, 6, 8.0f + 5.0f * lifeFraction,
                     phase * 240.0f, Color{235, 205, 255, 235});
        } else if (effect.sourceWeapon == WeaponType::FrostShards) {
            DrawRing(effect.position, effect.radius * 0.72f, effect.radius * 0.74f,
                     0.0f, 360.0f, 28, Color{215, 250, 255, 170});
        }
    }
}
