#pragma once

#include <array>
#include <vector>

#include "gameplay/Definitions.h"
#include "raylib.h"

class AudioManager;

struct Projectile {
    Vector2 position{};
    Vector2 velocity{};
    float damage = 0.0f;
    float lifetime = 0.0f;
    float radius = 0.0f;
    float knockback = 0.0f;
    float explosionRadius = 0.0f;
    float repeatHitCooldown = 0.0f;
    float trailTimer = 0.0f;
    float slowDuration = 0.0f;
    float slowMultiplier = 1.0f;
    float homingStrength = 0.0f;
    int remainingPierces = 0;
    int lastHitEnemy = -1;
    int homingTarget = -1;
    bool hitBoss = false;
    bool critical = false;
    ProjectileOwner owner = ProjectileOwner::Player;
    DamageSource source = DamageSource::PlayerProjectile;
    WeaponType sourceWeapon = WeaponType::ArcBolt;
    Color color = WHITE;
    bool explosive = false;
    bool active = false;
};

struct ExplosionEvent {
    Vector2 position{};
    float radius = 0.0f;
    float damage = 0.0f;
    float knockback = 0.0f;
    DamageSource source = DamageSource::PlayerArea;
    Color color = WHITE;
    bool critical = false;
};

class ProjectileManager {
public:
    ProjectileManager();
    void Reset();
    bool Spawn(const Projectile& specification);
    void SetAudioManager(AudioManager* audio) { audio_ = audio; }
    void Update(float deltaTime);
    void Draw() const;

    std::vector<Projectile>& Items() { return projectiles_; }
    const std::vector<Projectile>& Items() const { return projectiles_; }
    int ActiveCount() const { return playerActiveCount_ + enemyActiveCount_ + bossActiveCount_; }
    int ActiveCount(ProjectileOwner owner) const;
    void ClearOwner(ProjectileOwner owner);
    void Deactivate(Projectile& projectile, bool triggerExplosion = false);

    const std::array<ExplosionEvent, 128>& ExplosionEvents() const { return explosionEvents_; }
    int ExplosionEventCount() const { return explosionEventCount_; }
    void ClearExplosionEvents() { explosionEventCount_ = 0; }

private:
    void QueueExplosion(const Projectile& projectile);

    std::vector<Projectile> projectiles_;
    std::array<ExplosionEvent, 128> explosionEvents_{};
    int explosionEventCount_ = 0;
    int playerActiveCount_ = 0;
    int enemyActiveCount_ = 0;
    int bossActiveCount_ = 0;
    bool poolWarningEmitted_ = false;
    AudioManager* audio_ = nullptr;
};
