#pragma once

#include <algorithm>
#include "gameplay/Characters.h"
#include "raylib.h"

class WorldNavigation;

struct PlayerStats {
    float maxHP = 100.0f;
    float currentHP = 100.0f;
    float moveSpeed = 235.0f;
    float armor = 0.0f;
    float hpRegeneration = 0.0f;
    int level = 1;
    int currentXP = 0;
    int xpRequired = 12;
    float damageMultiplier = 1.0f;
    float cooldownMultiplier = 1.0f;
    float areaMultiplier = 1.0f;
    float projectileSpeedMultiplier = 1.0f;
    float durationMultiplier = 1.0f;
    float magnetRadius = 105.0f;
    float luck = 0.0f;
    float criticalChance = 0.05f;
    float criticalMultiplier = 1.75f;
    int globalPiercingBonus = 0;
    float temporaryAreaMultiplier = 1.0f;
    float lowHealthDamageBonus = 0.0f;
    float xpMultiplier = 1.0f;
    float slowEffectiveness = 1.0f;
    float controlStrength = 1.0f;
    int projectileCountBonus = 0;
};

class Player {
public:
    void Reset(CharacterId character = CharacterId::Hunter);
    void Update(float deltaTime, Rectangle arena, const WorldNavigation* navigation = nullptr);
    void SetPosition(Vector2 position) { position_ = position; }
    void SanitizeStats();
    bool TakeDamage(float amount);
    void HealToFull() { stats.currentHP = stats.maxHP; }
    void Heal(float amount) { stats.currentHP = std::min(stats.maxHP, stats.currentHP + amount); }
    void ToggleDebugInvulnerability() { debugInvulnerable_ = !debugInvulnerable_; }
    bool DebugInvulnerable() const { return debugInvulnerable_; }
    float LastDamageTaken() const { return lastDamageTaken_; }
    void Draw() const;

    Vector2 Position() const { return position_; }
    float Radius() const { return radius_; }
    bool IsDead() const { return stats.currentHP <= 0.0f; }
    Vector2 Facing() const { return facing_; }
    void SetFacing(Vector2 direction);
    bool IsMoving() const { return moving_; }
    bool IsDashing() const { return dashRemaining_ > 0.0f; }
    float DashCooldownRemaining() const { return dashCooldownRemaining_; }
    float DashCooldownDuration() const { return 8.0f; }
    bool ConsumeDashStarted() { const bool value = dashStarted_; dashStarted_ = false; return value; }
    bool TryPhaseDash(Vector2 direction);
    float InvulnerabilityRemaining() const { return invulnerabilityRemaining_; }
    void RegisterAoEHits(int hits);
    CharacterId Character() const { return character_; }
    const CharacterDefinition& CharacterDefinitionData() const { return GetCharacterDefinition(character_); }
    float TraitTimer() const { return traitTimer_; }
    bool FortifiedReady() const { return fortifiedReady_; }
    float VisualTime() const { return visualTime_; }

    PlayerStats stats;

private:
    Vector2 position_{0.0f, 0.0f};
    float radius_ = 18.0f;
    float invulnerabilityRemaining_ = 0.0f;
    bool debugInvulnerable_ = false;
    float lastDamageTaken_ = 0.0f;
    Vector2 facing_{0.0f, 1.0f};
    bool moving_ = false;
    float visualTime_ = 0.0f;
    float dashRemaining_ = 0.0f;
    float dashCooldownRemaining_ = 0.0f;
    Vector2 dashDirection_{0.0f, 1.0f};
    bool dashStarted_ = false;
    CharacterId character_ = CharacterId::Hunter;
    float traitTimer_ = 0.0f;
    float unharmedTimer_ = 0.0f;
    int traitHitAccumulator_ = 0;
    bool fortifiedReady_ = false;
};
