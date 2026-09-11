#include "entities/Player.h"

#include <algorithm>
#include <cmath>

#include "gameplay/Balance.h"
#include "game_modes/WorldNavigation.h"
#include "ui/VisualStyle.h"

void Player::Reset(CharacterId character) {
    stats = {};
    stats.maxHP = Balance::PlayerMaxHP;
    stats.currentHP = stats.maxHP;
    stats.moveSpeed = Balance::PlayerMoveSpeed;
    stats.xpRequired = Balance::XPRequiredForLevel(stats.level);
    stats.magnetRadius = Balance::PlayerMagnetRadius;
    position_ = {0.0f, 0.0f};
    radius_ = Balance::PlayerRadius;
    invulnerabilityRemaining_ = 0.0f;
    debugInvulnerable_ = false;
    lastDamageTaken_ = 0.0f;
    facing_ = {0.0f, 1.0f};
    moving_ = false;
    visualTime_ = 0.0f;
    dashRemaining_ = 0.0f;
    dashCooldownRemaining_ = 0.0f;
    dashDirection_ = facing_;
    dashStarted_ = false;
    character_ = character;
    traitTimer_ = 0.0f;
    unharmedTimer_ = 0.0f;
    traitHitAccumulator_ = 0;
    fortifiedReady_ = false;
    const CharacterDefinition& definition = GetCharacterDefinition(character_);
    stats.maxHP *= definition.maxHPMultiplier;
    stats.currentHP = stats.maxHP;
    stats.moveSpeed *= definition.moveSpeedMultiplier;
    stats.damageMultiplier *= definition.damageMultiplier;
    stats.areaMultiplier *= definition.areaMultiplier;
    stats.durationMultiplier *= definition.durationMultiplier;
    stats.armor += definition.armorBonus;
    stats.luck += definition.luckBonus;
    stats.criticalChance += definition.criticalBonus;
}

void Player::Update(float deltaTime, Rectangle arena, const WorldNavigation* navigation) {
    SanitizeStats();
    visualTime_ += deltaTime;
    invulnerabilityRemaining_ = std::max(0.0f, invulnerabilityRemaining_ - deltaTime);
    dashCooldownRemaining_ = std::max(0.0f, dashCooldownRemaining_ - deltaTime);
    stats.currentHP = std::min(stats.maxHP,
                               stats.currentHP + stats.hpRegeneration * deltaTime);
    traitTimer_ = std::max(0.0f, traitTimer_ - deltaTime);
    stats.temporaryAreaMultiplier = traitTimer_ > 0.0f ? 1.12f : 1.0f;
    unharmedTimer_ += deltaTime;
    if (CharacterDefinitionData().trait == CharacterTrait::Fortified && unharmedTimer_ >= 5.0f)
        fortifiedReady_ = true;

    Vector2 direction{};
    direction.x = static_cast<float>((IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) -
                                     (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)));
    direction.y = static_cast<float>((IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) -
                                     (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)));

    const float lengthSquared = direction.x * direction.x + direction.y * direction.y;
    moving_ = lengthSquared > 0.0f;
    if (lengthSquared > 0.0f) {
        const float inverseLength = 1.0f / std::sqrt(lengthSquared);
        direction.x *= inverseLength;
        direction.y *= inverseLength;
        facing_ = direction;
    }

    if (IsKeyPressed(KEY_SPACE)) TryPhaseDash(moving_ ? direction : facing_);
    const bool dashing = dashRemaining_ > 0.0f;
    if (dashing) dashRemaining_ = std::max(0.0f, dashRemaining_ - deltaTime);
    const Vector2 velocityDirection = dashing ? dashDirection_ : direction;
    const float speed = dashing ? 980.0f : stats.moveSpeed;
    const Vector2 desired{position_.x + velocityDirection.x * speed * deltaTime,
                          position_.y + velocityDirection.y * speed * deltaTime};
    if (navigation) position_ = navigation->ConstrainMovement(position_, desired, radius_);
    else {
        position_ = desired;
        position_.x = std::clamp(position_.x, arena.x + radius_, arena.x + arena.width - radius_);
        position_.y = std::clamp(position_.y, arena.y + radius_, arena.y + arena.height - radius_);
    }
}

bool Player::TryPhaseDash(Vector2 direction) {
    if (dashCooldownRemaining_ > 0.0f || IsDead()) return false;
    const float lengthSquared = direction.x * direction.x + direction.y * direction.y;
    if (lengthSquared <= 0.0001f) direction = facing_;
    else {
        const float inverseLength = 1.0f / std::sqrt(lengthSquared);
        direction.x *= inverseLength;
        direction.y *= inverseLength;
    }
    dashDirection_ = direction;
    dashRemaining_ = 0.15f;
    dashCooldownRemaining_ = 8.0f;
    invulnerabilityRemaining_ = std::max(invulnerabilityRemaining_, 0.20f);
    dashStarted_ = true;
    return true;
}

void Player::SetFacing(Vector2 direction) {
    const float lengthSquared = direction.x * direction.x + direction.y * direction.y;
    if (lengthSquared <= 0.0001f) return;
    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    facing_ = {direction.x * inverseLength, direction.y * inverseLength};
}

void Player::SanitizeStats() {
    auto finiteOr = [](float value, float fallback) { return std::isfinite(value) ? value : fallback; };
    stats.maxHP = std::clamp(finiteOr(stats.maxHP, Balance::PlayerMaxHP), 1.0f, 1000.0f);
    stats.currentHP = std::clamp(finiteOr(stats.currentHP, stats.maxHP), 0.0f, stats.maxHP);
    stats.moveSpeed = std::clamp(finiteOr(stats.moveSpeed, Balance::PlayerMoveSpeed), 80.0f, 600.0f);
    stats.armor = std::clamp(finiteOr(stats.armor, 0.0f), 0.0f, 50.0f);
    stats.hpRegeneration = std::clamp(finiteOr(stats.hpRegeneration, 0.0f), 0.0f, 20.0f);
    stats.damageMultiplier = std::clamp(finiteOr(stats.damageMultiplier, 1.0f), 0.1f, 10.0f);
    stats.cooldownMultiplier = std::clamp(finiteOr(stats.cooldownMultiplier, 1.0f), 0.20f, 3.0f);
    stats.areaMultiplier = std::clamp(finiteOr(stats.areaMultiplier, 1.0f), 0.25f, 5.0f);
    stats.projectileSpeedMultiplier = std::clamp(finiteOr(stats.projectileSpeedMultiplier, 1.0f), 0.25f, 5.0f);
    stats.durationMultiplier = std::clamp(finiteOr(stats.durationMultiplier, 1.0f), 0.25f, 5.0f);
    stats.magnetRadius = std::clamp(finiteOr(stats.magnetRadius, Balance::PlayerMagnetRadius), 30.0f, 1200.0f);
    stats.luck = std::clamp(finiteOr(stats.luck, 0.0f), 0.0f, 3.0f);
    stats.criticalChance = std::clamp(finiteOr(stats.criticalChance, 0.05f), 0.0f, 0.95f);
    stats.criticalMultiplier = std::clamp(finiteOr(stats.criticalMultiplier, 1.75f), 1.0f, 5.0f);
    stats.globalPiercingBonus = std::clamp(stats.globalPiercingBonus, 0, 100);
    stats.temporaryAreaMultiplier = std::clamp(finiteOr(stats.temporaryAreaMultiplier, 1.0f), 1.0f, 2.0f);
    stats.lowHealthDamageBonus = std::clamp(finiteOr(stats.lowHealthDamageBonus, 0.0f), 0.0f, 1.0f);
    stats.xpMultiplier = std::clamp(finiteOr(stats.xpMultiplier, 1.0f), 1.0f, 2.0f);
    stats.slowEffectiveness = std::clamp(finiteOr(stats.slowEffectiveness, 1.0f), 1.0f, 1.5f);
    stats.controlStrength = std::clamp(finiteOr(stats.controlStrength, 1.0f), 1.0f, 2.0f);
    stats.projectileCountBonus = std::clamp(stats.projectileCountBonus, 0, 2);
    stats.level = std::max(1, stats.level);
    stats.currentXP = std::max(0, stats.currentXP);
    stats.xpRequired = std::max(1, stats.xpRequired);
}

bool Player::TakeDamage(float amount) {
    lastDamageTaken_ = 0.0f;
    if (debugInvulnerable_ || invulnerabilityRemaining_ > 0.0f || IsDead()) {
        return false;
    }

    if (CharacterDefinitionData().trait == CharacterTrait::Fortified && fortifiedReady_) {
        amount *= 0.65f;
        fortifiedReady_ = false;
    }
    unharmedTimer_ = 0.0f;
    const float mitigatedDamage = std::max(1.0f, amount - stats.armor);
    lastDamageTaken_ = mitigatedDamage;
    stats.currentHP = std::max(0.0f, stats.currentHP - mitigatedDamage);
    invulnerabilityRemaining_ = Balance::PlayerInvulnerability;
    return true;
}

void Player::RegisterAoEHits(int hits) {
    if (CharacterDefinitionData().trait != CharacterTrait::BurningMomentum || hits <= 0) return;
    traitHitAccumulator_ += hits;
    if (traitHitAccumulator_ >= 5) {
        traitHitAccumulator_ %= 5;
        traitTimer_ = 3.0f;
        stats.temporaryAreaMultiplier = 1.12f;
    }
}

void Player::Draw() const {
    const bool flashing = invulnerabilityRemaining_ > 0.0f &&
                          static_cast<int>(invulnerabilityRemaining_ * 18.0f) % 2 == 0;
    VisualStyle::DrawPlayer(position_, radius_, facing_, moving_, flashing, visualTime_,
                            CharacterDefinitionData().color, IsDead());
}
