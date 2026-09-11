#include "systems/EnemyManager.h"

#include <algorithm>
#include <cmath>

#include "gameplay/Balance.h"
#include "game_modes/WorldNavigation.h"
#include "systems/ProjectileManager.h"
#include "ui/VisualStyle.h"

namespace {
float DistanceSquared(Vector2 a, Vector2 b) {
    const float x = a.x - b.x;
    const float y = a.y - b.y;
    return x * x + y * y;
}

Vector2 Normalized(Vector2 value) {
    const float lengthSquared = value.x * value.x + value.y * value.y;
    if (lengthSquared <= 0.0001f) return {};
    const float inverse = 1.0f / std::sqrt(lengthSquared);
    return {value.x * inverse, value.y * inverse};
}
} // namespace

EnemyManager::EnemyManager() {
    enemies_.reserve(Balance::EnemyPoolSize);
    enemies_.resize(Balance::EnemyPoolSize);
    queryScratch_.reserve(128);
    targetScratch_.reserve(600);
}

void EnemyManager::Reset() {
    for (Enemy& enemy : enemies_) enemy.active = false;
    grid_.Clear();
    activeCount_ = 0;
    deathEventCount_ = 0;
    blastEventCount_ = 0;
    damageFeedbackEventCount_ = 0;
    damageDealt_ = 0.0;
    poolWarningEmitted_ = false;
    healthyTargetDamageBonus_ = lowHealthDamageBonus_ = 0.0f;
    aoeHitsThisFrame_ = 0;
}

void EnemyManager::Update(float deltaTime, Vector2 playerPosition,
                          ProjectileManager& projectiles, float globalSpeedMultiplier,
                          const WorldNavigation* navigation) {
    aoeHitsThisFrame_ = 0;
    int globalSummons = ActiveCount(EnemyType::BoneMinion);
    for (std::size_t enemyIndex = 0; enemyIndex < enemies_.size(); ++enemyIndex) {
        Enemy& enemy = enemies_[enemyIndex];
        if (!enemy.active) continue;
        enemy.hitFlash = std::max(0.0f, enemy.hitFlash - deltaTime);
        enemy.orbitalHitCooldown = std::max(0.0f, enemy.orbitalHitCooldown - deltaTime);
        enemy.attackCooldown -= deltaTime;
        enemy.slowRemaining = std::max(0.0f, enemy.slowRemaining - deltaTime);
        if (enemy.slowRemaining <= 0.0f) enemy.slowMultiplier = 1.0f;
        if (enemy.type == EnemyType::ShieldedAcolyte && enemy.shieldHP <= 0.0f) {
            enemy.shieldCooldown -= deltaTime;
            if (enemy.shieldCooldown <= 0.0f) enemy.shieldHP = enemy.shieldMaxHP;
        }

        const EnemyDefinition& definition = GetEnemyDefinition(enemy.type);
        const Vector2 toPlayer{playerPosition.x - enemy.position.x,
                               playerPosition.y - enemy.position.y};
        const float distanceSquared = toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y;
        const float distance = std::sqrt(std::max(0.001f, distanceSquared));
        Vector2 movement{};
        float movementSpeed = enemy.speed;

        if (enemy.type == EnemyType::Wraith) {
            enemy.stateTimer -= deltaTime;
            if (enemy.intangible && enemy.stateTimer <= 0.0f) {
                enemy.intangible = false;
                enemy.contactEnabled = false;
                enemy.shieldCooldown = 0.45f;
                enemy.stateTimer = 2.6f;
            } else if (!enemy.intangible && enemy.stateTimer <= 0.0f) {
                enemy.intangible = true;
                enemy.contactEnabled = false;
                enemy.stateTimer = 1.15f;
            }
            if (!enemy.intangible && !enemy.contactEnabled) {
                enemy.shieldCooldown -= deltaTime;
                if (enemy.shieldCooldown <= 0.0f) enemy.contactEnabled = true;
            }
            movement = Normalized(toPlayer);
        } else if (enemy.type == EnemyType::Bomber) {
            if (enemy.telegraphing) {
                enemy.stateTimer -= deltaTime;
                if (enemy.stateTimer <= 0.0f) {
                    QueueBlast(enemy.position, 118.0f, 24.0f * enemy.specialDamageScale);
                    Deactivate(enemy, false);
                    continue;
                }
            } else if (distance <= 130.0f) {
                enemy.telegraphing = true;
                enemy.stateTimer = 0.9f;
            } else {
                movement = Normalized(toPlayer);
            }
        } else if (enemy.type == EnemyType::Cultist || enemy.type == EnemyType::Necromancer) {
            if (distance > definition.preferredRange + 55.0f) movement = Normalized(toPlayer);
            else if (distance < definition.preferredRange - 55.0f) {
                Vector2 away{-toPlayer.x, -toPlayer.y};
                movement = Normalized(away);
            } else {
                movement = Normalized({-toPlayer.y, toPlayer.x});
                movement.x *= 0.35f;
                movement.y *= 0.35f;
            }

            if (enemy.type == EnemyType::Necromancer && enemy.attackCooldown <= 0.0f) {
                int owned = 0;
                for (const Enemy& candidate : enemies_)
                    if (candidate.active && candidate.type == EnemyType::BoneMinion &&
                        candidate.summonerIndex == static_cast<int>(enemyIndex)) ++owned;
                const int summonCount = std::min(3 - owned, 2);
                for (int summon = 0; summon < summonCount && globalSummons < 24; ++summon) {
                    const float angle = (summon * 180.0f + enemyIndex * 37.0f) * DEG2RAD;
                    if (Spawn(EnemyType::BoneMinion,
                              {enemy.position.x + std::cos(angle) * 42.0f,
                               enemy.position.y + std::sin(angle) * 42.0f},
                              1, 1, 1, 0, false, SpawnSource::Necromancer,
                              EnemyVariant::None, static_cast<int>(enemyIndex))) ++globalSummons;
                }
                enemy.attackCooldown = definition.attackCooldown;
            } else if (enemy.type == EnemyType::Cultist && enemy.attackCooldown <= 0.0f && distance <= definition.attackRange) {
                const Vector2 direction = Normalized(toPlayer);
                Projectile shot{};
                shot.position = enemy.position;
                shot.velocity = {direction.x * definition.projectileSpeed,
                                 direction.y * definition.projectileSpeed};
                shot.damage = enemy.projectileDamage;
                shot.lifetime = 2.4f;
                shot.radius = 7.0f;
                shot.owner = ProjectileOwner::Enemy;
                shot.source = DamageSource::EnemyProjectile;
                shot.color = Color{255, 78, 120, 255};
                projectiles.Spawn(shot);
                enemy.attackCooldown = definition.attackCooldown;
            }
        } else if (enemy.type == EnemyType::GraveWarden) {
            // 0 chase, 1 charge windup, 2 charge, 3 recovery, 4 slam windup, 5 recovery.
            if (enemy.actionPhase == 1) {
                enemy.stateTimer -= deltaTime;
                if (enemy.stateTimer <= 0.0f) {
                    enemy.telegraphing = false;
                    enemy.actionPhase = 2;
                    enemy.stateTimer = 0.48f;
                }
            } else if (enemy.actionPhase == 2) {
                enemy.stateTimer -= deltaTime;
                movement = enemy.actionDirection;
                movementSpeed = 520.0f;
                if (enemy.stateTimer <= 0.0f) {
                    enemy.actionPhase = 3;
                    enemy.stateTimer = 0.62f;
                }
            } else if (enemy.actionPhase == 4) {
                enemy.stateTimer -= deltaTime;
                if (enemy.stateTimer <= 0.0f) {
                    QueueBlast(enemy.position, 150.0f, 20.0f * enemy.specialDamageScale);
                    for (int shotIndex = 0; shotIndex < 6; ++shotIndex) {
                        const float angle = shotIndex * (2.0f * PI / 6.0f);
                        Projectile shot{};
                        shot.position = enemy.position;
                        shot.velocity = {std::cos(angle) * 300.0f, std::sin(angle) * 300.0f};
                        shot.damage = 10.0f * enemy.specialDamageScale;
                        shot.lifetime = 1.5f; shot.radius = 8.0f;
                        shot.owner = ProjectileOwner::Enemy;
                        shot.source = DamageSource::EnemyProjectile;
                        shot.color = Color{235, 171, 76, 255};
                        projectiles.Spawn(shot);
                    }
                    enemy.telegraphing = false;
                    enemy.actionPhase = 5;
                    enemy.stateTimer = 0.72f;
                }
            } else if (enemy.actionPhase == 3 || enemy.actionPhase == 5) {
                enemy.stateTimer -= deltaTime;
                if (enemy.stateTimer <= 0.0f) {
                    enemy.actionPhase = 0;
                    enemy.attackCooldown = 1.8f;
                }
            } else if (enemy.attackCooldown <= 0.0f) {
                if (enemy.behaviorSequence++ % 2 == 0) {
                    enemy.actionPhase = 1;
                    enemy.actionDirection = Normalized(toPlayer);
                    enemy.stateTimer = 0.70f;
                } else {
                    enemy.actionPhase = 4;
                    enemy.stateTimer = 0.90f;
                }
                enemy.telegraphing = true;
            } else movement = Normalized(toPlayer);
        } else if (enemy.type == EnemyType::VoidStalker) {
            if (enemy.telegraphing) {
                enemy.stateTimer -= deltaTime;
                if (enemy.stateTimer <= 0.0f) {
                    if (enemy.actionPhase++ % 2 == 0) {
                        enemy.position.x += enemy.actionDirection.x * 250.0f;
                        enemy.position.y += enemy.actionDirection.y * 250.0f;
                    } else {
                        const float baseAngle = std::atan2(toPlayer.y, toPlayer.x);
                        for (int shotIndex = -2; shotIndex <= 2; ++shotIndex) {
                            const float angle = baseAngle + shotIndex * 0.15f;
                            Projectile shot{};
                            shot.position = enemy.position;
                            shot.velocity = {std::cos(angle) * definition.projectileSpeed,
                                             std::sin(angle) * definition.projectileSpeed};
                            shot.damage = enemy.projectileDamage;
                            shot.lifetime = 2.5f; shot.radius = 8.0f;
                            shot.owner = ProjectileOwner::Enemy;
                            shot.source = DamageSource::EnemyProjectile;
                            shot.color = Color{213, 79, 255, 255};
                            projectiles.Spawn(shot);
                        }
                    }
                    enemy.telegraphing = false;
                    enemy.attackCooldown = 2.6f;
                }
            } else if (enemy.attackCooldown <= 0.0f) {
                enemy.actionDirection = Normalized(toPlayer);
                enemy.telegraphing = true;
                enemy.stateTimer = 0.68f;
            } else if (distance > definition.preferredRange) movement = Normalized(toPlayer);
        } else {
            movement = Normalized(toPlayer);
        }

        const Vector2 directToward = Normalized(toPlayer);
        if (navigation && !(enemy.type == EnemyType::GraveWarden && enemy.actionPhase == 2) &&
            movement.x * directToward.x + movement.y * directToward.y > 0.5f)
            movement = navigation->DirectionToward(enemy.position, playerPosition);
        const Vector2 previous = enemy.position;
        const Vector2 desired{enemy.position.x + (movement.x * movementSpeed * globalSpeedMultiplier * enemy.slowMultiplier + enemy.knockbackVelocity.x) * deltaTime,
                              enemy.position.y + (movement.y * movementSpeed * globalSpeedMultiplier * enemy.slowMultiplier + enemy.knockbackVelocity.y) * deltaTime};
        enemy.position = navigation ? navigation->ConstrainMovement(previous, desired, enemy.radius) : desired;
        if (enemy.type == EnemyType::GraveWarden && enemy.actionPhase == 2 && navigation &&
            DistanceSquared(previous, enemy.position) < DistanceSquared(previous, desired) * 0.15f) {
            enemy.actionPhase = 3;
            enemy.stateTimer = 0.62f;
        }
        const float damping = std::max(0.0f, 1.0f - deltaTime * 8.0f);
        enemy.knockbackVelocity.x *= damping;
        enemy.knockbackVelocity.y *= damping;

        const float farX = enemy.position.x - playerPosition.x;
        const float farY = enemy.position.y - playerPosition.y;
        if (!enemy.miniboss && farX * farX + farY * farY > 2200.0f * 2200.0f) {
            enemy.active = false;
            --activeCount_;
        }
    }

    grid_.Rebuild(enemies_);
    SeparateEnemies();
    if (navigation) ConstrainToNavigation(*navigation);
    grid_.Rebuild(enemies_);
}

void EnemyManager::ConstrainToNavigation(const WorldNavigation& navigation) {
    for (Enemy& enemy : enemies_) if (enemy.active)
        if (!navigation.IsWalkable(enemy.position, enemy.radius))
            enemy.position = navigation.FindNearestWalkablePosition(enemy.position, enemy.radius);
}

void EnemyManager::Draw() const {
    const float time = static_cast<float>(GetTime());
    for (const Enemy& enemy : enemies_) {
        if (!enemy.active) continue;
        VisualStyle::DrawEnemy(enemy, time);
        if (enemy.telegraphing) {
            if (enemy.type == EnemyType::VoidStalker) {
                DrawLineEx(enemy.position,
                           {enemy.position.x + enemy.actionDirection.x * 250.0f,
                            enemy.position.y + enemy.actionDirection.y * 250.0f},
                           12.0f, Color{220, 70, 255, 75});
                DrawLineEx(enemy.position,
                           {enemy.position.x + enemy.actionDirection.x * 250.0f,
                            enemy.position.y + enemy.actionDirection.y * 250.0f},
                           2.0f, Color{245, 190, 255, 240});
            } else if (enemy.type == EnemyType::GraveWarden && enemy.actionPhase == 1) {
                DrawLineEx(enemy.position,
                           {enemy.position.x + enemy.actionDirection.x * 370.0f,
                            enemy.position.y + enemy.actionDirection.y * 370.0f},
                           18.0f, Color{245, 155, 60, 65});
                DrawLineEx(enemy.position,
                           {enemy.position.x + enemy.actionDirection.x * 370.0f,
                            enemy.position.y + enemy.actionDirection.y * 370.0f},
                           3.0f, Color{255, 220, 140, 235});
            } else {
                const float duration = enemy.type == EnemyType::GraveWarden ? 0.90f : 0.68f;
                const float progress = 1.0f - enemy.stateTimer / duration;
                const float targetRadius = enemy.type == EnemyType::GraveWarden
                                               ? 150.0f
                                               : 118.0f;
                DrawRing(enemy.position, targetRadius * std::clamp(progress, 0.1f, 1.0f),
                         targetRadius * std::clamp(progress, 0.1f, 1.0f) + 4.0f,
                         time * 90.0f, time * 90.0f + 280.0f, 32,
                         Color{255, 75, 50, 220});
            }
        }

        if (enemy.hp < enemy.maxHP) {
            const float width = enemy.radius * 1.8f;
            DrawRectangleRec({enemy.position.x - width * 0.5f, enemy.position.y - enemy.radius - 8.0f,
                              width, 3.0f}, Color{35, 35, 40, 220});
            DrawRectangleRec({enemy.position.x - width * 0.5f, enemy.position.y - enemy.radius - 8.0f,
                              width * (enemy.hp / enemy.maxHP), 3.0f}, LIME);
        }
    }
}

bool EnemyManager::Spawn(EnemyType type, Vector2 position, float hpScale,
                         float damageScale, float speedScale, float xpScale, bool elite,
                         SpawnSource source, EnemyVariant variant, int summonerIndex) {
    for (Enemy& enemy : enemies_) {
        if (enemy.active) continue;
        const EnemyDefinition& definition = GetEnemyDefinition(type);
        enemy = {};
        enemy.position = position;
        enemy.radius = definition.radius * (elite ? 1.25f : 1.0f);
        enemy.hp = definition.maxHP * hpScale * (elite ? 3.0f : 1.0f);
        enemy.maxHP = enemy.hp;
        enemy.speed = definition.moveSpeed * speedScale * (elite ? 1.04f : 1.0f);
        enemy.contactDamage = definition.contactDamage * damageScale * (elite ? 1.5f : 1.0f);
        enemy.projectileDamage = definition.projectileDamage * damageScale * (elite ? 1.5f : 1.0f);
        enemy.specialDamageScale = damageScale * (elite ? 1.5f : 1.0f);
        enemy.xpReward = std::max(1, static_cast<int>(definition.xpReward * xpScale *
                                                     (elite ? 3.0f : 1.0f)));
        enemy.attackCooldown = static_cast<float>(GetRandomValue(30, 100)) * 0.01f;
        enemy.type = type;
        enemy.spawnSource = source;
        enemy.variant = variant;
        enemy.summonerIndex = summonerIndex;
        enemy.elite = elite;
        enemy.miniboss = type == EnemyType::GraveWarden || type == EnemyType::VoidStalker;
        if (enemy.miniboss) enemy.attackCooldown = 0.0f;
        if (type == EnemyType::ShieldedAcolyte) {
            enemy.shieldMaxHP = enemy.maxHP * 0.42f;
            enemy.shieldHP = enemy.shieldMaxHP;
        }
        if (type == EnemyType::Wraith) enemy.stateTimer = 2.1f;
        if (variant == EnemyVariant::Frenzied) { enemy.speed *= 1.16f; enemy.contactDamage *= 1.10f; }
        else if (variant == EnemyVariant::Armored) { enemy.hp *= 1.28f; enemy.maxHP = enemy.hp; enemy.speed *= 0.94f; }
        else if (variant == EnemyVariant::Empowered) { enemy.contactDamage *= 1.22f; enemy.projectileDamage *= 1.22f; }
        enemy.active = true;
        ++activeCount_;
        return true;
    }
    if (!poolWarningEmitted_) {
        TraceLog(LOG_WARNING, "Enemy pool full; spawn skipped");
        poolWarningEmitted_ = true;
    }
    return false;
}

bool EnemyManager::SpawnMiniboss(EnemyType type, Vector2 position, float difficultyScale) {
    if (HasActiveMiniboss() || (type != EnemyType::GraveWarden && type != EnemyType::VoidStalker))
        return false;
    return Spawn(type, position, difficultyScale, difficultyScale, 1.0f, 1.0f, false,
                 SpawnSource::Miniboss);
}

const Enemy* EnemyManager::FindNearest(Vector2 origin, float maxRange) const {
    const Enemy* nearest = nullptr;
    float nearestDistance = maxRange * maxRange;
    targetScratch_.clear();
    float radius = std::min(256.0f, maxRange);
    while (radius <= maxRange && nearest == nullptr) {
        grid_.QueryCircle(origin, radius, targetScratch_);
        for (int index : targetScratch_) {
            const Enemy& enemy = enemies_[static_cast<std::size_t>(index)];
            if (!enemy.active || enemy.intangible) continue;
            const float distance = DistanceSquared(origin, enemy.position);
            if (distance < nearestDistance) { nearestDistance = distance; nearest = &enemy; }
        }
        if (radius >= 3000.0f || radius >= maxRange) break;
        radius = std::min(radius * 2.0f, std::min(3000.0f, maxRange));
    }
    return nearest;
}

void EnemyManager::RebuildGrid() { grid_.Rebuild(enemies_); }

void EnemyManager::CullToLimit(int limit) {
    limit = std::max(0, limit);
    for (auto iterator = enemies_.rbegin(); iterator != enemies_.rend() && activeCount_ > limit;
         ++iterator) {
        if (!iterator->active || iterator->miniboss) continue;
        iterator->active = false;
        --activeCount_;
    }
    RebuildGrid();
}

void EnemyManager::DespawnBySource(SpawnSource source) {
    for (Enemy& enemy : enemies_) if (enemy.active && enemy.spawnSource == source) {
        enemy.active = false;
        --activeCount_;
    }
    RebuildGrid();
}

void EnemyManager::DespawnMiniboss() {
    for (Enemy& enemy : enemies_) if (enemy.active && enemy.miniboss) {
        enemy.active = false;
        --activeCount_;
    }
    RebuildGrid();
}

void EnemyManager::QueryCircle(Vector2 position, float radius, std::vector<int>& results) const {
    grid_.QueryCircle(position, radius, results);
}

int EnemyManager::ActiveCount(EnemyType type) const {
    int count = 0;
    for (const Enemy& enemy : enemies_) if (enemy.active && enemy.type == type) ++count;
    return count;
}

int EnemyManager::EliteCount() const {
    int count = 0;
    for (const Enemy& enemy : enemies_) if (enemy.active && enemy.elite) ++count;
    return count;
}

int EnemyManager::VariantCount() const {
    int count = 0;
    for (const Enemy& enemy : enemies_)
        if (enemy.active && enemy.variant != EnemyVariant::None) ++count;
    return count;
}

bool EnemyManager::HasActiveMiniboss() const { return ActiveMiniboss() != nullptr; }

const Enemy* EnemyManager::ActiveMiniboss() const {
    for (const Enemy& enemy : enemies_) if (enemy.active && enemy.miniboss) return &enemy;
    return nullptr;
}

bool EnemyManager::Damage(std::size_t index, float amount, Vector2 direction,
                          float knockback, DamageSource source, bool critical) {
    if (index >= enemies_.size() || !enemies_[index].active) return false;
    Enemy& enemy = enemies_[index];
    if (enemy.intangible) return false;
    const bool playerDamage = source == DamageSource::PlayerProjectile ||
                              source == DamageSource::PlayerArea ||
                              source == DamageSource::PlayerOrbital;
    if (playerDamage && enemy.hp > enemy.maxHP * 0.80f) amount *= 1.0f + healthyTargetDamageBonus_;
    if (playerDamage && enemy.hp < enemy.maxHP * 0.30f)
        amount *= 1.0f + lowHealthDamageBonus_ * (enemy.miniboss ? 0.5f : 1.0f);
    if (source == DamageSource::PlayerArea) ++aoeHitsThisFrame_;
    if (enemy.shieldHP > 0.0f) {
        const float absorbed = std::min(enemy.shieldHP, amount * 0.68f);
        enemy.shieldHP -= absorbed;
        amount -= absorbed;
        if (enemy.shieldHP <= 0.0f) enemy.shieldCooldown = 5.0f;
    }
    damageDealt_ += std::min(enemy.hp, amount);
    enemy.hp -= amount;
    enemy.hitFlash = 0.08f;
    if (damageFeedbackEventCount_ < static_cast<int>(damageFeedbackEvents_.size()))
        damageFeedbackEvents_[static_cast<std::size_t>(damageFeedbackEventCount_++)] =
            {enemy.position, amount, critical};
    const float resistance = GetEnemyDefinition(enemy.type).knockbackResistance;
    enemy.knockbackVelocity.x += direction.x * knockback * (1.0f - resistance);
    enemy.knockbackVelocity.y += direction.y * knockback * (1.0f - resistance);
    if (enemy.hp > 0.0f) return false;
    Deactivate(enemy, enemy.type == EnemyType::Bomber);
    return true;
}

void EnemyManager::ApplySlow(std::size_t index, float duration, float movementMultiplier) {
    if (index >= enemies_.size()) return;
    Enemy& enemy = enemies_[index];
    if (!enemy.active || enemy.intangible) return;
    movementMultiplier = std::clamp(movementMultiplier, 0.55f, 1.0f);
    if (enemy.miniboss) movementMultiplier = 1.0f - (1.0f - movementMultiplier) * 0.45f;
    enemy.slowMultiplier = std::min(enemy.slowMultiplier, movementMultiplier);
    enemy.slowRemaining = std::max(enemy.slowRemaining, duration * (enemy.miniboss ? 0.55f : 1.0f));
}

void EnemyManager::ApplyPull(std::size_t index, Vector2 center, float strength) {
    if (index >= enemies_.size()) return;
    Enemy& enemy = enemies_[index];
    if (!enemy.active || enemy.intangible) return;
    Vector2 direction{center.x - enemy.position.x, center.y - enemy.position.y};
    const float length = std::sqrt(std::max(0.001f, direction.x * direction.x + direction.y * direction.y));
    const float resistance = enemy.miniboss ? 0.20f : 1.0f;
    enemy.knockbackVelocity.x += direction.x / length * strength * resistance;
    enemy.knockbackVelocity.y += direction.y / length * strength * resistance;
}

void EnemyManager::SeparateEnemies() {
    for (std::size_t first = 0; first < enemies_.size(); ++first) {
        if (!enemies_[first].active) continue;
        grid_.QueryCircle(enemies_[first].position, enemies_[first].radius * 2.2f, queryScratch_);
        for (int rawSecond : queryScratch_) {
            const std::size_t second = static_cast<std::size_t>(rawSecond);
            if (second <= first) continue;
            if (!enemies_[second].active) continue;
            Vector2 delta{enemies_[second].position.x - enemies_[first].position.x,
                          enemies_[second].position.y - enemies_[first].position.y};
            const float minimum = (enemies_[first].radius + enemies_[second].radius) * 0.72f;
            const float distanceSquared = delta.x * delta.x + delta.y * delta.y;
            if (distanceSquared <= 0.01f || distanceSquared >= minimum * minimum) continue;
            const float distance = std::sqrt(distanceSquared);
            const float push = (minimum - distance) * 0.18f;
            delta.x /= distance;
            delta.y /= distance;
            enemies_[first].position.x -= delta.x * push;
            enemies_[first].position.y -= delta.y * push;
            enemies_[second].position.x += delta.x * push;
            enemies_[second].position.y += delta.y * push;
        }
    }
}

void EnemyManager::QueueDeath(const Enemy& enemy) {
    if (deathEventCount_ >= static_cast<int>(deathEvents_.size())) return;
    deathEvents_[static_cast<std::size_t>(deathEventCount_++)] =
        {enemy.position, enemy.xpReward, enemy.type, enemy.spawnSource, enemy.elite, enemy.miniboss};
}

void EnemyManager::QueueBlast(Vector2 position, float radius, float damage) {
    if (blastEventCount_ >= static_cast<int>(blastEvents_.size())) return;
    blastEvents_[static_cast<std::size_t>(blastEventCount_++)] = {position, radius, damage};
}

void EnemyManager::Deactivate(Enemy& enemy, bool earlyBomberDeath) {
    QueueDeath(enemy);
    if (earlyBomberDeath)
        QueueBlast(enemy.position, 72.0f, 12.0f * enemy.specialDamageScale);
    enemy.active = false;
    --activeCount_;
}
