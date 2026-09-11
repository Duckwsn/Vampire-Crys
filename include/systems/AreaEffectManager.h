#pragma once

#include <vector>

#include "gameplay/Definitions.h"
#include "raylib.h"

class EnemyManager;
class BossManager;

struct AreaEffect {
    Vector2 position{};
    float radius = 0.0f;
    float damage = 0.0f;
    float lifetime = 0.0f;
    float totalLifetime = 0.0f;
    float tickInterval = 0.25f;
    float tickTimer = 0.0f;
    float knockback = 0.0f;
    float pullStrength = 0.0f;
    float slowDuration = 0.0f;
    float slowMultiplier = 1.0f;
    int ticksRemaining = 1;
    DamageSource source = DamageSource::PlayerArea;
    WeaponType sourceWeapon = WeaponType::ArcBolt;
    Color color = WHITE;
    bool followPlayer = false;
    bool critical = false;
    bool active = false;
};

class AreaEffectManager {
public:
    AreaEffectManager();
    void Reset();
    bool Spawn(const AreaEffect& specification);
    void Update(float deltaTime, Vector2 playerPosition, EnemyManager& enemies,
                BossManager* bosses = nullptr);
    void Draw() const;
    int ActiveCount() const { return activeCount_; }
    const std::vector<AreaEffect>& Items() const { return effects_; }

private:
    std::vector<AreaEffect> effects_;
    std::vector<int> queryScratch_;
    int activeCount_ = 0;
    bool poolWarningEmitted_ = false;
};
