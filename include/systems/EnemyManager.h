#pragma once

#include <array>
#include <cstddef>
#include <limits>
#include <vector>

#include "gameplay/Definitions.h"
#include "raylib.h"
#include "systems/SpatialGrid.h"

class ProjectileManager;
class WorldNavigation;

struct Enemy {
    Vector2 position{};
    Vector2 knockbackVelocity{};
    float radius = 0.0f;
    float hp = 0.0f;
    float maxHP = 0.0f;
    float speed = 0.0f;
    float contactDamage = 0.0f;
    float projectileDamage = 0.0f;
    float specialDamageScale = 1.0f;
    float hitFlash = 0.0f;
    float attackCooldown = 0.0f;
    float stateTimer = 0.0f;
    float orbitalHitCooldown = 0.0f;
    float shieldHP = 0.0f;
    float shieldMaxHP = 0.0f;
    float shieldCooldown = 0.0f;
    float slowRemaining = 0.0f;
    float slowMultiplier = 1.0f;
    Vector2 actionDirection{};
    int summonerIndex = -1;
    int actionPhase = 0;
    int behaviorSequence = 0;
    int xpReward = 0;
    EnemyType type = EnemyType::Ghoul;
    EnemyVariant variant = EnemyVariant::None;
    SpawnSource spawnSource = SpawnSource::Director;
    bool telegraphing = false;
    bool intangible = false;
    bool contactEnabled = true;
    bool miniboss = false;
    bool elite = false;
    bool active = false;
};

struct EnemyDeathEvent {
    Vector2 position{};
    int xpReward = 0;
    EnemyType type = EnemyType::Ghoul;
    SpawnSource spawnSource = SpawnSource::Director;
    bool elite = false;
    bool miniboss = false;
};

struct EnemyBlastEvent {
    Vector2 position{};
    float radius = 0.0f;
    float damage = 0.0f;
};

class EnemyManager {
public:
    EnemyManager();
    void Reset();
    void Update(float deltaTime, Vector2 playerPosition, ProjectileManager& projectiles,
                float globalSpeedMultiplier = 1.0f,
                const WorldNavigation* navigation = nullptr);
    void ConstrainToNavigation(const WorldNavigation& navigation);
    void Draw() const;
    bool Spawn(EnemyType type, Vector2 position, float hpScale = 1.0f,
               float damageScale = 1.0f, float speedScale = 1.0f,
               float xpScale = 1.0f, bool elite = false,
               SpawnSource source = SpawnSource::Director,
               EnemyVariant variant = EnemyVariant::None, int summonerIndex = -1);
    bool SpawnMiniboss(EnemyType type, Vector2 position, float difficultyScale = 1.0f);
    bool HasActiveMiniboss() const;
    const Enemy* ActiveMiniboss() const;
    bool CanContactPlayer(const Enemy& enemy) const { return enemy.contactEnabled && !enemy.intangible; }
    const Enemy* FindNearest(Vector2 origin,
                             float maxRange = std::numeric_limits<float>::max()) const;
    bool Damage(std::size_t index, float amount, Vector2 direction, float knockback,
                DamageSource source, bool critical = false);
    void ApplySlow(std::size_t index, float duration, float movementMultiplier);
    void ApplyPull(std::size_t index, Vector2 center, float strength);
    void SetPlayerDamageBonuses(float healthyTarget, float lowHealthTarget) {
        healthyTargetDamageBonus_ = healthyTarget;
        lowHealthDamageBonus_ = lowHealthTarget;
    }
    int AoEHitsThisFrame() const { return aoeHitsThisFrame_; }
    void RebuildGrid();
    void QueryCircle(Vector2 position, float radius, std::vector<int>& results) const;
    void CullToLimit(int limit);
    void DespawnBySource(SpawnSource source);
    void DespawnMiniboss();

    std::vector<Enemy>& Items() { return enemies_; }
    const std::vector<Enemy>& Items() const { return enemies_; }
    int ActiveCount() const { return activeCount_; }
    int ActiveCount(EnemyType type) const;
    int EliteCount() const;
    int VariantCount() const;
    int GridCellCount() const { return grid_.ActiveCellCount(); }
    int GridEntryCount() const { return grid_.EntryCount(); }
    double DamageDealt() const { return damageDealt_; }
    const std::array<EnemyDeathEvent, 600>& DeathEvents() const { return deathEvents_; }
    int DeathEventCount() const { return deathEventCount_; }
    void ClearDeathEvents() { deathEventCount_ = 0; }
    const std::array<EnemyBlastEvent, 128>& BlastEvents() const { return blastEvents_; }
    int BlastEventCount() const { return blastEventCount_; }
    void ClearBlastEvents() { blastEventCount_ = 0; }
    const std::array<DamageFeedbackEvent, 256>& DamageFeedbackEvents() const { return damageFeedbackEvents_; }
    int DamageFeedbackEventCount() const { return damageFeedbackEventCount_; }
    void ClearDamageFeedbackEvents() { damageFeedbackEventCount_ = 0; }

private:
    void SeparateEnemies();
    void QueueDeath(const Enemy& enemy);
    void QueueBlast(Vector2 position, float radius, float damage);
    void Deactivate(Enemy& enemy, bool earlyBomberDeath);

    std::vector<Enemy> enemies_;
    SpatialGrid grid_;
    std::vector<int> queryScratch_;
    mutable std::vector<int> targetScratch_;
    std::array<EnemyDeathEvent, 600> deathEvents_{};
    std::array<EnemyBlastEvent, 128> blastEvents_{};
    std::array<DamageFeedbackEvent, 256> damageFeedbackEvents_{};
    int activeCount_ = 0;
    int deathEventCount_ = 0;
    int blastEventCount_ = 0;
    int damageFeedbackEventCount_ = 0;
    double damageDealt_ = 0.0;
    bool poolWarningEmitted_ = false;
    float healthyTargetDamageBonus_ = 0.0f;
    float lowHealthDamageBonus_ = 0.0f;
    int aoeHitsThisFrame_ = 0;
};
