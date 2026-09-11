#include "systems/BossManager.h"

#include <algorithm>
#include <cmath>

#include "entities/Player.h"
#include "gameplay/Balance.h"
#include "game_modes/WorldNavigation.h"
#include "systems/ProjectileManager.h"
#include "ui/VisualStyle.h"

namespace {
Vector2 Normalize(Vector2 value) {
    const float length = std::sqrt(std::max(0.0001f, value.x * value.x + value.y * value.y));
    return {value.x / length, value.y / length};
}
float DistanceSquared(Vector2 a, Vector2 b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}
}

void BossManager::Reset() {
    boss_ = {};
    telegraphs_.Reset();
    attackPositions_ = {};
    attackDirections_ = {};
    attackPositionCount_ = 0;
    attackDirectionCount_ = 0;
    attackSequence_ = 0;
    introTimer_ = 0.0f;
    damageToPlayerThisFrame_ = 0.0f;
    damageTaken_ = 0.0;
    damageFeedbackEventCount_ = 0;
    deathPending_ = false;
    deathEvent_ = {};
    healthyTargetDamageBonus_ = 0.0f;
    lowHealthDamageBonus_ = 0.0f;
    spawnHpMultiplier_ = spawnDamageMultiplier_ = 1.0f;
}

void BossManager::Update(float deltaTime, Player& player,
                         ProjectileManager& projectiles, const WorldNavigation* navigation) {
    navigation_ = navigation;
    damageToPlayerThisFrame_ = telegraphs_.Update(deltaTime, player);
    introTimer_ = std::max(0.0f, introTimer_ - deltaTime);

    if (!boss_.alive) return;

    boss_.orbitalHitCooldown = std::max(0.0f, boss_.orbitalHitCooldown - deltaTime);
    boss_.slowRemaining = std::max(0.0f, boss_.slowRemaining - deltaTime);
    if (boss_.slowRemaining <= 0.0f) boss_.slowMultiplier = 1.0f;
    boss_.hitFlash = std::max(0.0f, boss_.hitFlash - deltaTime);
    boss_.stateTimer -= deltaTime;
    const float healthFraction = boss_.currentHP / boss_.maxHP;
    const float phaseThreshold = boss_.type == BossType::VoidHeraldAscended ? 0.40f : 0.50f;
    if (boss_.phase == 1 && healthFraction <= phaseThreshold) {
        boss_.phase = 2;
        introTimer_ = 0.7f;
    }

    const float contactReach = boss_.radius + player.Radius();
    if (DistanceSquared(boss_.position, player.Position()) <= contactReach * contactReach &&
        player.TakeDamage(boss_.contactDamage))
        damageToPlayerThisFrame_ += player.LastDamageTaken();

    if (boss_.currentState == BossState::Spawn) {
        if (boss_.stateTimer <= 0.0f) { boss_.currentState = BossState::Move; boss_.stateTimer = 1.6f; }
        return;
    }
    if (boss_.currentState == BossState::Move) {
        const Vector2 previousPosition = boss_.position;
        const Vector2 toPlayer{player.Position().x - boss_.position.x,
                               player.Position().y - boss_.position.y};
        const float distance = std::sqrt(std::max(0.0001f, DistanceSquared(boss_.position, player.Position())));
        const Vector2 toward{toPlayer.x / distance, toPlayer.y / distance};
        const Vector2 lateral{-toward.y, toward.x};
        const float radial = distance > 430.0f ? 0.65f : (distance < 270.0f ? -0.75f : 0.0f);
        const float side = (attackSequence_ % 2 == 0 ? 1.0f : -1.0f) * 0.55f;
        boss_.position.x += (toward.x * radial + lateral.x * side) * boss_.moveSpeed * boss_.slowMultiplier * deltaTime;
        boss_.position.y += (toward.y * radial + lateral.y * side) * boss_.moveSpeed * boss_.slowMultiplier * deltaTime;
        boss_.position = navigation_ ? navigation_->ConstrainMovement(previousPosition, boss_.position, boss_.radius)
                                     : ClampToArena(boss_.position);
        boss_.direction = toward;
        if (boss_.stateTimer <= 0.0f) boss_.currentState = BossState::AttackSelection;
        return;
    }
    if (boss_.currentState == BossState::AttackSelection) {
        BeginAttack(player.Position());
        return;
    }
    if (boss_.currentState == BossState::Telegraph) {
        if (boss_.stateTimer <= 0.0f) {
            ExecuteAttack(projectiles);
            boss_.currentState = BossState::AttackExecution;
            boss_.stateTimer = boss_.currentAttack == BossAttack::FlamePools ? 0.45f : 0.75f;
        }
        return;
    }
    if (boss_.currentState == BossState::AttackExecution && boss_.stateTimer <= 0.0f) {
        EnterRecovery();
        return;
    }
    if (boss_.currentState == BossState::Recovery && boss_.stateTimer <= 0.0f) {
        boss_.currentState = BossState::Move;
        boss_.stateTimer = boss_.type == BossType::VoidHeraldAscended ? 0.85f : 1.25f;
    }
}

void BossManager::Spawn(BossType type, Vector2 playerPosition) {
    boss_ = {};
    boss_.type = type;
    boss_.name = type == BossType::FlameWyrm ? "FLAME WYRM" :
                 (type == BossType::VoidHerald ? "VOID HERALD" : "VOID HERALD ASCENDED");
    boss_.maxHP = type == BossType::FlameWyrm ? 4200.0f :
                  (type == BossType::VoidHerald ? 8200.0f : 15500.0f);
    boss_.maxHP *= spawnHpMultiplier_;
    boss_.currentHP = boss_.maxHP;
    boss_.moveSpeed = type == BossType::FlameWyrm ? 72.0f :
                      (type == BossType::VoidHerald ? 62.0f : 78.0f);
    boss_.contactDamage = type == BossType::FlameWyrm ? 14.0f :
                          (type == BossType::VoidHerald ? 16.0f : 20.0f);
    boss_.contactDamage *= spawnDamageMultiplier_;
    boss_.radius = type == BossType::FlameWyrm ? 58.0f : 52.0f;
    const Vector2 requested{playerPosition.x + 520.0f, playerPosition.y};
    boss_.position = navigation_ ? navigation_->FindNearestWalkablePosition(requested, boss_.radius)
                                 : ClampToArena(requested);
    boss_.direction = {-1.0f, 0.0f};
    boss_.currentState = BossState::Spawn;
    boss_.stateTimer = 1.0f;
    boss_.alive = true;
    attackSequence_ = 0;
    introTimer_ = 1.8f;
    telegraphs_.Reset();
}

void BossManager::SpawnDebug(BossType type, Vector2 playerPosition) {
    StartEncounter(type, playerPosition);
}

bool BossManager::StartEncounter(BossType type, Vector2 playerPosition) {
    if (boss_.alive || deathPending_) return false;
    Spawn(type, playerPosition);
    return true;
}

void BossManager::BeginAttack(Vector2 playerPosition) {
    boss_.direction = Normalize({playerPosition.x - boss_.position.x,
                                 playerPosition.y - boss_.position.y});
    attackPositionCount_ = 0;
    attackDirectionCount_ = 0;
    const bool useFirst = attackSequence_++ % 2 == 0;
    const float warning = boss_.type == BossType::VoidHeraldAscended ? 0.72f : 0.95f;

    if (boss_.type == BossType::FlameWyrm) {
        boss_.currentAttack = useFirst ? BossAttack::FireBreath : BossAttack::FlamePools;
        if (useFirst) {
            Telegraph warningShape{};
            warningShape.type = TelegraphType::Cone;
            warningShape.position = boss_.position;
            warningShape.direction = boss_.direction;
            warningShape.range = 390.0f;
            warningShape.angleDegrees = boss_.phase == 2 ? 66.0f : 58.0f;
            warningShape.duration = 1.0f;
            warningShape.color = ORANGE;
            telegraphs_.Spawn(warningShape);
            boss_.stateTimer = 1.0f;
        } else {
            attackPositionCount_ = boss_.phase == 2 ? 5 : 4;
            for (int index = 0; index < attackPositionCount_; ++index) {
                const float angle = (2.0f * PI * index) / attackPositionCount_ + attackSequence_ * 0.41f;
                const float radius = index == 0 ? 0.0f : 115.0f + 45.0f * (index % 2);
                attackPositions_[static_cast<std::size_t>(index)] = ClampToArena({
                    playerPosition.x + std::cos(angle) * radius,
                    playerPosition.y + std::sin(angle) * radius});
                Telegraph circle{};
                circle.type = TelegraphType::Circle;
                circle.position = attackPositions_[static_cast<std::size_t>(index)];
                circle.radius = 64.0f;
                circle.duration = 1.05f;
                circle.color = ORANGE;
                telegraphs_.Spawn(circle);
            }
            boss_.stateTimer = 1.05f;
        }
    } else {
        boss_.currentAttack = useFirst ? BossAttack::RadialBarrage : BossAttack::VoidBeams;
        if (useFirst) {
            Telegraph radial{};
            radial.type = TelegraphType::Circle;
            radial.position = boss_.position;
            radial.radius = 95.0f;
            radial.duration = warning;
            radial.color = PURPLE;
            telegraphs_.Spawn(radial);
            boss_.stateTimer = warning;
        } else {
            attackDirectionCount_ = boss_.type == BossType::VoidHeraldAscended
                                        ? (boss_.phase == 2 ? 4 : 3)
                                        : (boss_.phase == 2 ? 3 : 2);
            for (int index = 0; index < attackDirectionCount_; ++index) {
                const float base = std::atan2(boss_.direction.y, boss_.direction.x);
                const float angle = base + (index - (attackDirectionCount_ - 1) * 0.5f) * 0.48f;
                attackDirections_[static_cast<std::size_t>(index)] = {std::cos(angle), std::sin(angle)};
                Telegraph line{};
                line.type = TelegraphType::Line;
                line.position = boss_.position;
                line.direction = attackDirections_[static_cast<std::size_t>(index)];
                line.range = 1050.0f;
                line.width = 42.0f;
                line.duration = warning;
                line.color = VIOLET;
                telegraphs_.Spawn(line);
            }
            boss_.stateTimer = warning;
        }
    }
    boss_.currentState = BossState::Telegraph;
}

void BossManager::ExecuteAttack(ProjectileManager& projectiles) {
    if (boss_.currentAttack == BossAttack::FireBreath) {
        Telegraph breath{};
        breath.type = TelegraphType::Cone;
        breath.position = boss_.position;
        breath.direction = boss_.direction;
        breath.range = 390.0f;
        breath.angleDegrees = boss_.phase == 2 ? 66.0f : 58.0f;
        breath.duration = 0.65f;
        breath.damage = (boss_.phase == 2 ? 18.0f : 15.0f) * spawnDamageMultiplier_;
        breath.tickInterval = 0.32f;
        breath.color = RED;
        breath.dangerous = true;
        telegraphs_.Spawn(breath);
    } else if (boss_.currentAttack == BossAttack::FlamePools) {
        for (int index = 0; index < attackPositionCount_; ++index) {
            Telegraph pool{};
            pool.type = TelegraphType::Circle;
            pool.position = attackPositions_[static_cast<std::size_t>(index)];
            pool.radius = 64.0f;
            pool.duration = boss_.phase == 2 ? 4.8f : 4.0f;
            pool.damage = (boss_.phase == 2 ? 12.0f : 10.0f) * spawnDamageMultiplier_;
            pool.tickInterval = 0.65f;
            pool.color = Color{255, 76, 35, 255};
            pool.dangerous = true;
            telegraphs_.Spawn(pool);
        }
    } else if (boss_.currentAttack == BossAttack::RadialBarrage) {
        int count = boss_.type == BossType::VoidHeraldAscended ? 20 : (boss_.phase == 2 ? 16 : 12);
        SpawnRadialRing(projectiles, count, 0.0f);
        if (boss_.phase == 2 || boss_.type == BossType::VoidHeraldAscended)
            SpawnRadialRing(projectiles, count, PI / static_cast<float>(count));
    } else if (boss_.currentAttack == BossAttack::VoidBeams) {
        for (int index = 0; index < attackDirectionCount_; ++index) {
            Telegraph beam{};
            beam.type = TelegraphType::Line;
            beam.position = boss_.position;
            beam.direction = attackDirections_[static_cast<std::size_t>(index)];
            beam.range = 1050.0f;
            beam.width = 42.0f;
            beam.duration = 0.75f;
            beam.damage = (boss_.type == BossType::VoidHeraldAscended ? 20.0f : 16.0f) *
                          spawnDamageMultiplier_;
            beam.tickInterval = 0.40f;
            beam.color = Color{190, 70, 255, 255};
            beam.dangerous = true;
            telegraphs_.Spawn(beam);
        }
    }
}

void BossManager::SpawnRadialRing(ProjectileManager& projectiles, int count, float angleOffset) {
    for (int index = 0; index < count; ++index) {
        const float angle = angleOffset + 2.0f * PI * static_cast<float>(index) / static_cast<float>(count);
        const Vector2 direction{std::cos(angle), std::sin(angle)};
        Projectile projectile{};
        projectile.position = boss_.position;
        projectile.velocity = {direction.x * 245.0f, direction.y * 245.0f};
        projectile.damage = (boss_.type == BossType::VoidHeraldAscended ? 16.0f : 12.0f) *
                            spawnDamageMultiplier_;
        projectile.lifetime = 5.0f;
        projectile.radius = 8.0f;
        projectile.owner = ProjectileOwner::Boss;
        projectile.source = DamageSource::BossAttack;
        projectile.color = Color{190, 80, 255, 255};
        projectiles.Spawn(projectile);
    }
}

void BossManager::EnterRecovery() {
    boss_.currentState = BossState::Recovery;
    const float base = boss_.type == BossType::VoidHeraldAscended ? 0.85f : 1.25f;
    boss_.stateTimer = boss_.phase == 2 ? base * 0.82f : base;
}

bool BossManager::Damage(float amount, DamageSource source, bool critical) {
    if (!boss_.alive || amount <= 0.0f) return false;
    const bool playerDamage = source == DamageSource::PlayerProjectile ||
                              source == DamageSource::PlayerArea ||
                              source == DamageSource::PlayerOrbital;
    if (playerDamage && boss_.currentHP > boss_.maxHP * 0.80f)
        amount *= 1.0f + healthyTargetDamageBonus_;
    if (playerDamage && boss_.currentHP < boss_.maxHP * 0.30f)
        amount *= 1.0f + lowHealthDamageBonus_ * 0.5f;
    const float applied = std::min(amount, boss_.currentHP);
    boss_.currentHP -= applied;
    boss_.hitFlash = 0.10f;
    if (damageFeedbackEventCount_ < static_cast<int>(damageFeedbackEvents_.size()))
        damageFeedbackEvents_[static_cast<std::size_t>(damageFeedbackEventCount_++)] =
            {boss_.position, amount, critical};
    damageTaken_ += applied;
    if (boss_.currentHP > 0.0f) return false;
    boss_.currentState = BossState::Death;
    boss_.alive = false;
    telegraphs_.Reset();
    deathEvent_ = {boss_.position, boss_.type, boss_.type == BossType::VoidHeraldAscended};
    deathPending_ = true;
    return true;
}

void BossManager::ApplySlow(float duration, float movementMultiplier) {
    if (!boss_.alive) return;
    movementMultiplier = std::clamp(movementMultiplier, 0.88f, 1.0f);
    boss_.slowMultiplier = std::min(boss_.slowMultiplier, movementMultiplier);
    boss_.slowRemaining = std::max(boss_.slowRemaining, duration * 0.40f);
}

BossDeathEvent BossManager::ConsumeDeathEvent() {
    deathPending_ = false;
    return deathEvent_;
}

void BossManager::ClearHazards(ProjectileManager& projectiles) {
    telegraphs_.Reset();
    projectiles.ClearOwner(ProjectileOwner::Boss);
}

Vector2 BossManager::ClampToArena(Vector2 position) const {
    if (navigation_) return navigation_->FindNearestWalkablePosition(position, boss_.radius);
    const float margin = boss_.radius + 12.0f;
    position.x = std::clamp(position.x, Balance::Arena.x + margin,
                            Balance::Arena.x + Balance::Arena.width - margin);
    position.y = std::clamp(position.y, Balance::Arena.y + margin,
                            Balance::Arena.y + Balance::Arena.height - margin);
    return position;
}

void BossManager::Draw() const {
    telegraphs_.Draw();
    if (!boss_.alive) return;
    VisualStyle::DrawBoss(boss_, static_cast<float>(GetTime()));
}
