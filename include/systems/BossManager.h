#pragma once

#include <algorithm>
#include <array>

#include "gameplay/Definitions.h"
#include "raylib.h"
#include "systems/TelegraphManager.h"

class Player;
class ProjectileManager;
class WorldNavigation;

enum class BossType { FlameWyrm, VoidHerald, VoidHeraldAscended };
enum class BossState { Inactive, Spawn, Move, AttackSelection, Telegraph, AttackExecution, Recovery, Death };
enum class BossAttack { None, FireBreath, FlamePools, RadialBarrage, VoidBeams };

struct Boss {
    BossType type = BossType::FlameWyrm;
    const char* name = "";
    Vector2 position{};
    Vector2 direction{1.0f, 0.0f};
    float maxHP = 1.0f;
    float currentHP = 1.0f;
    float moveSpeed = 1.0f;
    float contactDamage = 1.0f;
    float radius = 48.0f;
    float stateTimer = 0.0f;
    float attackCooldown = 0.0f;
    float orbitalHitCooldown = 0.0f;
    float hitFlash = 0.0f;
    float slowRemaining = 0.0f;
    float slowMultiplier = 1.0f;
    int phase = 1;
    BossState currentState = BossState::Inactive;
    BossAttack currentAttack = BossAttack::None;
    bool alive = false;
};

struct BossDeathEvent {
    Vector2 position{};
    BossType type = BossType::FlameWyrm;
    bool finalBoss = false;
};

class BossManager {
public:
    void Reset();
    void Update(float deltaTime, Player& player, ProjectileManager& projectiles,
                const WorldNavigation* navigation = nullptr);
    void Draw() const;
    bool StartEncounter(BossType type, Vector2 playerPosition);
    void SpawnDebug(BossType type, Vector2 playerPosition);
    bool Damage(float amount, DamageSource source, bool critical = false);
    void ApplySlow(float duration, float movementMultiplier);
    void SetPlayerDamageBonuses(float healthyTarget, float lowHealthTarget) {
        healthyTargetDamageBonus_ = healthyTarget;
        lowHealthDamageBonus_ = lowHealthTarget;
    }
    void SetSpawnMultipliers(float hp, float damage) {
        spawnHpMultiplier_ = std::clamp(hp, 0.5f, 5.0f);
        spawnDamageMultiplier_ = std::clamp(damage, 0.5f, 3.0f);
    }
    void SetNavigation(const WorldNavigation* navigation) { navigation_ = navigation; }
    void ClearHazards(ProjectileManager& projectiles);

    bool IsActive() const { return boss_.alive; }
    const Boss& ActiveBoss() const { return boss_; }
    Boss& ActiveBoss() { return boss_; }
    bool HasDeathEvent() const { return deathPending_; }
    BossDeathEvent ConsumeDeathEvent();
    float IntroRemaining() const { return introTimer_; }
    int TelegraphCount() const { return telegraphs_.ActiveCount(); }
    double DamageTaken() const { return damageTaken_; }
    const std::array<DamageFeedbackEvent, 128>& DamageFeedbackEvents() const { return damageFeedbackEvents_; }
    int DamageFeedbackEventCount() const { return damageFeedbackEventCount_; }
    void ClearDamageFeedbackEvents() { damageFeedbackEventCount_ = 0; }
    float DamageToPlayerThisFrame() const { return damageToPlayerThisFrame_; }

private:
    void Spawn(BossType type, Vector2 playerPosition);
    void BeginAttack(Vector2 playerPosition);
    void ExecuteAttack(ProjectileManager& projectiles);
    void SpawnRadialRing(ProjectileManager& projectiles, int count, float angleOffset);
    void EnterRecovery();
    Vector2 ClampToArena(Vector2 position) const;

    Boss boss_{};
    TelegraphManager telegraphs_;
    std::array<Vector2, 6> attackPositions_{};
    std::array<Vector2, 4> attackDirections_{};
    int attackPositionCount_ = 0;
    int attackDirectionCount_ = 0;
    int attackSequence_ = 0;
    float introTimer_ = 0.0f;
    float damageToPlayerThisFrame_ = 0.0f;
    double damageTaken_ = 0.0;
    bool deathPending_ = false;
    BossDeathEvent deathEvent_{};
    std::array<DamageFeedbackEvent, 128> damageFeedbackEvents_{};
    int damageFeedbackEventCount_ = 0;
    float healthyTargetDamageBonus_ = 0.0f;
    float lowHealthDamageBonus_ = 0.0f;
    float spawnHpMultiplier_ = 1.0f;
    float spawnDamageMultiplier_ = 1.0f;
    const WorldNavigation* navigation_ = nullptr;
};
