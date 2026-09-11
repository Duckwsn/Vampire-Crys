#include "systems/WeaponManager.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "entities/Player.h"
#include "gameplay/Balance.h"
#include "systems/AreaEffectManager.h"
#include "systems/AudioManager.h"
#include "systems/BossManager.h"
#include "systems/EnemyManager.h"
#include "systems/ProjectileManager.h"

namespace {
Vector2 DirectionAtAngle(float radians) { return {std::cos(radians), std::sin(radians)}; }

float DirectionAngle(Vector2 from, Vector2 to) {
    return std::atan2(to.y - from.y, to.x - from.x);
}

bool Overlaps(Vector2 a, float ar, Vector2 b, float br) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float radius = ar + br;
    return dx * dx + dy * dy <= radius * radius;
}

float DistanceSquared(Vector2 a, Vector2 b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}
} // namespace

void WeaponManager::Reset(WeaponType startingWeapon) {
    weapons_ = {};
    passives_ = {};
    weaponCount_ = 1;
    passiveCount_ = 0;
    weaponSlotLimit_ = MaxWeaponSlots;
    weapons_[0] = {startingWeapon, 1, 0.25f, 0.0f};
    chainSegments_ = {};
    sweepVisuals_ = {};
    if (queryScratch_.capacity() < 600) queryScratch_.reserve(600);
}

void WeaponManager::TickTimers(float deltaTime) {
    for (ChainSegment& segment : chainSegments_)
        segment.lifetime = std::max(0.0f, segment.lifetime - deltaTime);
    for (SweepVisual& sweep : sweepVisuals_)
        sweep.lifetime = std::max(0.0f, sweep.lifetime - deltaTime);
    for (int index = 0; index < weaponCount_; ++index)
        weapons_[static_cast<std::size_t>(index)].cooldownTimer -= deltaTime;
}

void WeaponManager::Update(float deltaTime, Player& player, EnemyManager& enemies,
                           ProjectileManager& projectiles, AreaEffectManager& areas,
                           BossManager* bosses) {
    TickTimers(deltaTime);
    for (int index = 0; index < weaponCount_; ++index) {
        WeaponInstance& weapon = weapons_[static_cast<std::size_t>(index)];
        if (weapon.evolved != EvolvedWeaponType::None) {
            UpdateEvolvedWeapon(weapon, deltaTime, player, enemies, projectiles, areas, bosses);
            continue;
        }
        switch (weapon.type) {
            case WeaponType::ArcBolt:
                if (weapon.cooldownTimer <= 0.0f)
                    FireLinearWeapon(weapon, player.Position(), player.stats, enemies,
                                     projectiles, 10.0f, false, bosses);
                break;
            case WeaponType::FlameRing:
                UpdateFlameRing(weapon, player.Position(), player.stats, areas);
                break;
            case WeaponType::GuardianOrbs:
                UpdateGuardianOrbs(weapon, deltaTime, player.Position(), player.stats, enemies, bosses);
                break;
            case WeaponType::SpectralFan:
                if (weapon.cooldownTimer <= 0.0f) {
                    const WeaponStats stats = GetWeaponStats(weapon.type, weapon.level);
                    FireLinearWeapon(weapon, player.Position(), player.stats, enemies,
                                     projectiles, stats.orbitSpeed, false, bosses);
                }
                break;
            case WeaponType::VoidLance:
                if (weapon.cooldownTimer <= 0.0f)
                    FireLinearWeapon(weapon, player.Position(), player.stats, enemies,
                                     projectiles, 0.0f, false, bosses);
                break;
            case WeaponType::ThunderCannon:
                if (weapon.cooldownTimer <= 0.0f)
                    FireLinearWeapon(weapon, player.Position(), player.stats, enemies,
                                     projectiles, 12.0f, true, bosses);
                break;
            case WeaponType::SoulScythe:
                if (weapon.cooldownTimer <= 0.0f) FireSoulScythe(weapon, player, enemies, bosses, false);
                break;
            case WeaponType::FrostShards:
                if (weapon.cooldownTimer <= 0.0f)
                    FireFrostShards(weapon, player.Position(), player.stats, enemies, projectiles, areas, bosses, false);
                break;
            case WeaponType::BloodNeedles:
                if (weapon.cooldownTimer <= 0.0f)
                    FireBloodNeedles(weapon, player.Position(), player.stats, enemies, projectiles, bosses, false);
                break;
            case WeaponType::GravityWell:
                if (weapon.cooldownTimer <= 0.0f)
                    SpawnGravityWell(weapon, player.Position(), player.stats, enemies, areas, bosses, false);
                break;
            case WeaponType::Count:
                break;
        }
    }
}

void WeaponManager::UpdateManual(float deltaTime, const AttackRequest& request, Player& player,
                                 EnemyManager& enemies, ProjectileManager& projectiles,
                                 AreaEffectManager& areas, BossManager* bosses) {
    TickTimers(deltaTime);
    player.SetFacing(request.aimDirection);
    for (int slot = 0; slot < weaponCount_ && slot < weaponSlotLimit_; ++slot) {
        WeaponInstance& weapon = weapons_[static_cast<std::size_t>(slot)];
        const bool held = slot == 0 ? request.primaryHeld : request.secondaryHeld;
        if (weapon.type == WeaponType::GuardianOrbs)
            UpdateGuardianOrbs(weapon, deltaTime, player.Position(), player.stats, enemies, bosses,
                               held && weapon.cooldownTimer <= 0.0f);
        if (held && weapon.cooldownTimer <= 0.0f)
            FireManualWeapon(slot, request, player, enemies, projectiles, areas, bosses);
    }
}

void WeaponManager::FireManualWeapon(int slot, const AttackRequest& request, Player& player,
                                     EnemyManager& enemies, ProjectileManager& projectiles,
                                     AreaEffectManager& areas, BossManager* bosses) {
    if (slot < 0 || slot >= weaponCount_) return;
    WeaponInstance& weapon = weapons_[static_cast<std::size_t>(slot)];
    const bool evolved = weapon.evolved != EvolvedWeaponType::None;
    if (weapon.type == WeaponType::FlameRing) {
        if (evolved) UpdateEvolvedWeapon(weapon, 0.0f, player, enemies, projectiles, areas, bosses);
        else UpdateFlameRing(weapon, player.Position(), player.stats, areas);
        return;
    }
    if (weapon.type == WeaponType::GuardianOrbs) {
        const WeaponStats base = GetWeaponStats(weapon.type, weapon.level);
        weapon.cooldownTimer = (evolved ? 1.8f : 0.32f) * player.stats.cooldownMultiplier;
        if (evolved) {
            for (int index = 0; index < 10; ++index) {
                const float angle = 2.0f * PI * index / 10.0f;
                Projectile shot{};
                shot.position = player.Position();
                shot.velocity = {std::cos(angle) * 460.0f, std::sin(angle) * 460.0f};
                shot.damage = RollDamage(base.damage * 1.65f, player.stats, &shot.critical);
                shot.lifetime = 1.5f; shot.radius = 6.0f; shot.remainingPierces = 1;
                shot.owner = ProjectileOwner::Player; shot.source = DamageSource::PlayerOrbital;
                shot.sourceWeapon = weapon.type; shot.color = GOLD;
                projectiles.Spawn(shot);
            }
        }
        return;
    }
    if (weapon.type == WeaponType::SoulScythe) {
        FireSoulScythe(weapon, player, enemies, bosses, evolved);
        return;
    }

    WeaponStats stats = GetWeaponStats(weapon.type, weapon.level);
    float damageScale = evolved ? 1.55f : 1.0f;
    float cooldownScale = evolved ? 0.72f : 1.0f;
    float areaScale = evolved ? 1.28f : 1.0f;
    int bonusProjectiles = evolved ? 2 : 0;
    int bonusPiercing = evolved ? 2 : 0;
    if (weapon.type == WeaponType::GravityWell) {
        Vector2 offset{request.aimWorld.x - player.Position().x,
                       request.aimWorld.y - player.Position().y};
        const float length = std::sqrt(std::max(0.0001f, offset.x * offset.x + offset.y * offset.y));
        const float rangeScale = std::min(1.0f, 460.0f / length);
        AreaEffect effect{};
        effect.position = {player.Position().x + offset.x * rangeScale,
                           player.Position().y + offset.y * rangeScale};
        effect.radius = stats.area * player.stats.areaMultiplier * areaScale;
        effect.damage = RollDamage(stats.damage * damageScale, player.stats, &effect.critical);
        effect.lifetime = effect.totalLifetime = stats.duration * player.stats.durationMultiplier;
        effect.tickInterval = effect.lifetime / std::max(1, stats.ticks);
        effect.ticksRemaining = stats.ticks; effect.pullStrength = evolved ? 210.0f : 145.0f;
        effect.source = DamageSource::PlayerArea; effect.sourceWeapon = weapon.type;
        effect.color = GetWeaponDefinition(weapon.type).color;
        if (areas.Spawn(effect)) weapon.cooldownTimer = stats.cooldown * player.stats.cooldownMultiplier * cooldownScale;
        return;
    }

    float spread = 0.0f;
    if (weapon.type == WeaponType::ArcBolt) spread = 10.0f;
    else if (weapon.type == WeaponType::SpectralFan) spread = stats.orbitSpeed;
    else if (weapon.type == WeaponType::ThunderCannon) spread = 12.0f;
    else if (weapon.type == WeaponType::FrostShards) spread = evolved ? 34.0f : 23.0f;
    else if (weapon.type == WeaponType::BloodNeedles) spread = 8.0f;
    int count = std::clamp(stats.projectileCount + player.stats.projectileCountBonus + bonusProjectiles, 1, 16);
    if (weapon.type == WeaponType::VoidLance) count = std::min(count, 3);
    const float center = std::atan2(request.aimDirection.y, request.aimDirection.x);
    bool fired = false;
    for (int index = 0; index < count; ++index) {
        const float fraction = count == 1 ? 0.5f : static_cast<float>(index) / (count - 1);
        const float angle = center + (fraction - 0.5f) * spread * DEG2RAD;
        const Vector2 direction{std::cos(angle), std::sin(angle)};
        Projectile shot{};
        shot.position = player.Position();
        shot.velocity = {direction.x * stats.projectileSpeed * player.stats.projectileSpeedMultiplier,
                         direction.y * stats.projectileSpeed * player.stats.projectileSpeedMultiplier};
        shot.damage = RollDamage(stats.damage * damageScale, player.stats, &shot.critical);
        shot.lifetime = stats.duration * player.stats.durationMultiplier;
        shot.radius = (weapon.type == WeaponType::ThunderCannon ? 8.0f : stats.area * areaScale);
        shot.knockback = stats.knockback;
        shot.remainingPierces = stats.piercing + player.stats.globalPiercingBonus + bonusPiercing;
        shot.owner = ProjectileOwner::Player; shot.source = DamageSource::PlayerProjectile;
        shot.sourceWeapon = weapon.type; shot.color = GetWeaponDefinition(weapon.type).color;
        shot.explosive = weapon.type == WeaponType::ThunderCannon;
        shot.explosionRadius = shot.explosive ? stats.area * player.stats.areaMultiplier * areaScale : 0.0f;
        if (weapon.type == WeaponType::FrostShards) { shot.slowDuration = evolved ? 2.4f : 1.6f; shot.slowMultiplier = evolved ? 0.60f : 0.74f; }
        if (weapon.type == WeaponType::BloodNeedles) {
            const Enemy* target = enemies.FindNearest(player.Position(), 700.0f);
            if (target) {
                shot.homingTarget = static_cast<int>(target - enemies.Items().data());
                shot.homingStrength = evolved ? 4.8f : 3.2f;
            }
        }
        fired |= projectiles.Spawn(shot);
    }
    if (fired) {
        weapon.cooldownTimer = stats.cooldown * player.stats.cooldownMultiplier * cooldownScale;
        if (audio_) audio_->Play(weapon.type == WeaponType::ThunderCannon ? AudioCue::Thunder : AudioCue::ArcBolt);
    }
}

void WeaponManager::FireLinearWeapon(WeaponInstance& weapon, Vector2 origin,
                                     const PlayerStats& playerStats,
                                     const EnemyManager& enemies,
                                     ProjectileManager& projectiles,
                                     float spreadDegrees, bool explosive, BossManager* bosses) {
    const Enemy* target = enemies.FindNearest(origin);
    Vector2 targetPosition{};
    bool hasTarget = false;
    float targetDistanceSquared = std::numeric_limits<float>::max();
    if (target != nullptr) {
        targetPosition = target->position;
        const float dx = targetPosition.x - origin.x;
        const float dy = targetPosition.y - origin.y;
        targetDistanceSquared = dx * dx + dy * dy;
        hasTarget = true;
    }
    if (bosses != nullptr && bosses->IsActive()) {
        const Vector2 position = bosses->ActiveBoss().position;
        const float dx = position.x - origin.x;
        const float dy = position.y - origin.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared < targetDistanceSquared) {
            targetPosition = position;
            hasTarget = true;
        }
    }
    if (!hasTarget) {
        weapon.cooldownTimer = 0.08f;
        return;
    }

    WeaponStats stats = GetWeaponStats(weapon.type, weapon.level);
    if (weapon.evolved == EvolvedWeaponType::TempestCannon) {
        stats.damage *= 1.65f;
        stats.area *= 1.25f;
        stats.cooldown *= 0.72f;
        stats.projectileSpeed *= 1.18f;
    }
    const float centerAngle = DirectionAngle(origin, targetPosition);
    const float spreadRadians = spreadDegrees * DEG2RAD;
    bool fired = false;
    const int effectiveCount = std::min(16, stats.projectileCount + playerStats.projectileCountBonus);
    for (int shot = 0; shot < effectiveCount; ++shot) {
        const float fraction = effectiveCount == 1
                                   ? 0.5f
                                   : static_cast<float>(shot) / static_cast<float>(effectiveCount - 1);
        const float angle = centerAngle + (fraction - 0.5f) * spreadRadians;
        const Vector2 direction = DirectionAtAngle(angle);
        Projectile projectile{};
        projectile.position = origin;
        projectile.velocity = {direction.x * stats.projectileSpeed * playerStats.projectileSpeedMultiplier,
                               direction.y * stats.projectileSpeed * playerStats.projectileSpeedMultiplier};
        projectile.damage = RollDamage(stats.damage, playerStats, &projectile.critical);
        projectile.lifetime = stats.duration * playerStats.durationMultiplier;
        projectile.radius = explosive ? 8.0f : stats.area;
        projectile.knockback = stats.knockback;
        projectile.explosionRadius = explosive ? stats.area * playerStats.areaMultiplier * playerStats.temporaryAreaMultiplier : 0.0f;
        projectile.remainingPierces = stats.piercing + playerStats.globalPiercingBonus;
        projectile.owner = ProjectileOwner::Player;
        projectile.source = DamageSource::PlayerProjectile;
        projectile.sourceWeapon = weapon.type;
        projectile.color = GetWeaponDefinition(weapon.type).color;
        projectile.explosive = explosive;
        fired |= projectiles.Spawn(projectile);
    }
    if (fired) weapon.cooldownTimer = stats.cooldown * playerStats.cooldownMultiplier;
}

void WeaponManager::UpdateFlameRing(WeaponInstance& weapon, Vector2 origin,
                                    const PlayerStats& stats, AreaEffectManager& areas) {
    if (weapon.cooldownTimer > 0.0f) return;
    const WeaponStats weaponStats = GetWeaponStats(weapon.type, weapon.level);
    AreaEffect effect{};
    effect.position = origin;
    effect.radius = weaponStats.area * stats.areaMultiplier * stats.temporaryAreaMultiplier;
    effect.damage = RollDamage(weaponStats.damage, stats, &effect.critical);
    effect.lifetime = weaponStats.duration * stats.durationMultiplier;
    effect.totalLifetime = effect.lifetime;
    effect.tickInterval = effect.lifetime / static_cast<float>(weaponStats.ticks);
    effect.ticksRemaining = weaponStats.ticks;
    effect.knockback = weaponStats.knockback;
    effect.source = DamageSource::PlayerArea;
    effect.sourceWeapon = weapon.type;
    effect.color = GetWeaponDefinition(weapon.type).color;
    effect.followPlayer = true;
    if (areas.Spawn(effect)) {
        weapon.cooldownTimer = weaponStats.cooldown * stats.cooldownMultiplier;
        if (audio_) audio_->Play(AudioCue::Flame);
    }
}

void WeaponManager::UpdateEvolvedWeapon(WeaponInstance& weapon, float deltaTime, Player& player,
                                        EnemyManager& enemies, ProjectileManager& projectiles,
                                        AreaEffectManager& areas, BossManager* bosses) {
    const Vector2 origin = player.Position();
    const PlayerStats& stats = player.stats;
    if (weapon.evolved == EvolvedWeaponType::ReapersCovenant) {
        if (weapon.cooldownTimer <= 0.0f) FireSoulScythe(weapon, player, enemies, bosses, true);
        return;
    }
    if (weapon.evolved == EvolvedWeaponType::AbsoluteZero) {
        if (weapon.cooldownTimer <= 0.0f)
            FireFrostShards(weapon, origin, stats, enemies, projectiles, areas, bosses, true);
        return;
    }
    if (weapon.evolved == EvolvedWeaponType::CrimsonSwarm) {
        if (weapon.cooldownTimer <= 0.0f)
            FireBloodNeedles(weapon, origin, stats, enemies, projectiles, bosses, true);
        return;
    }
    if (weapon.evolved == EvolvedWeaponType::Singularity) {
        if (weapon.cooldownTimer <= 0.0f)
            SpawnGravityWell(weapon, origin, stats, enemies, areas, bosses, true);
        return;
    }
    if (weapon.evolved == EvolvedWeaponType::InfernoHalo) {
        if (weapon.cooldownTimer > 0.0f) return;
        AreaEffect halo{};
        halo.position = origin;
        halo.radius = 168.0f * stats.areaMultiplier * stats.temporaryAreaMultiplier;
        halo.damage = RollDamage(17.0f, stats, &halo.critical);
        halo.lifetime = 1.15f * stats.durationMultiplier;
        halo.totalLifetime = halo.lifetime;
        halo.tickInterval = 0.25f;
        halo.ticksRemaining = 5;
        halo.knockback = 12.0f;
        halo.source = DamageSource::PlayerArea;
        halo.sourceWeapon = WeaponType::FlameRing;
        halo.color = Color{255, 68, 24, 255};
        halo.followPlayer = true;
        if (areas.Spawn(halo)) {
            weapon.cooldownTimer = 0.78f * stats.cooldownMultiplier;
            if (audio_) audio_->Play(AudioCue::Flame);
        }
        return;
    }
    if (weapon.evolved == EvolvedWeaponType::CelestialGuard) {
        UpdateGuardianOrbs(weapon, deltaTime, origin, stats, enemies, bosses);
        if (weapon.cooldownTimer > 0.0f) return;
        for (int index = 0; index < 10; ++index) {
            const float angle = 2.0f * PI * static_cast<float>(index) / 10.0f;
            const Vector2 direction = DirectionAtAngle(angle);
            Projectile shot{};
            shot.position = origin;
            shot.velocity = {direction.x * 460.0f * stats.projectileSpeedMultiplier,
                             direction.y * 460.0f * stats.projectileSpeedMultiplier};
            shot.damage = RollDamage(15.0f, stats, &shot.critical);
            shot.lifetime = 1.5f * stats.durationMultiplier;
            shot.radius = 6.0f * stats.areaMultiplier * stats.temporaryAreaMultiplier;
            shot.remainingPierces = 1 + stats.globalPiercingBonus;
            shot.owner = ProjectileOwner::Player;
            shot.source = DamageSource::PlayerOrbital;
            shot.sourceWeapon = WeaponType::GuardianOrbs;
            shot.color = GOLD;
            projectiles.Spawn(shot);
        }
        weapon.cooldownTimer = 2.6f * stats.cooldownMultiplier;
        return;
    }
    if (weapon.evolved == EvolvedWeaponType::PhantomBarrage) {
        if (weapon.cooldownTimer > 0.0f) return;
        for (int index = 0; index < 18; ++index) {
            const float angle = 2.0f * PI * static_cast<float>(index) / 18.0f + weapon.phase;
            const Vector2 direction = DirectionAtAngle(angle);
            Projectile shot{};
            shot.position = origin;
            shot.velocity = {direction.x * 520.0f * stats.projectileSpeedMultiplier,
                             direction.y * 520.0f * stats.projectileSpeedMultiplier};
            shot.damage = RollDamage(14.0f, stats, &shot.critical);
            shot.lifetime = 1.65f * stats.durationMultiplier;
            shot.radius = 5.5f;
            shot.remainingPierces = 1 + stats.globalPiercingBonus;
            shot.owner = ProjectileOwner::Player;
            shot.source = DamageSource::PlayerProjectile;
            shot.sourceWeapon = WeaponType::SpectralFan;
            shot.color = Color{125, 255, 225, 255};
            projectiles.Spawn(shot);
        }
        weapon.phase += 0.17f;
        weapon.cooldownTimer = 1.15f * stats.cooldownMultiplier;
        return;
    }
    if (weapon.evolved == EvolvedWeaponType::AbyssSpear) {
        if (weapon.cooldownTimer > 0.0f) return;
        const Enemy* enemy = enemies.FindNearest(origin);
        Vector2 target = enemy != nullptr ? enemy->position : Vector2{};
        bool found = enemy != nullptr;
        if (bosses != nullptr && bosses->IsActive()) {
            const Vector2 bossPosition = bosses->ActiveBoss().position;
            if (!found || DistanceSquared(origin, target) > DistanceSquared(origin, bossPosition)) {
                target = bossPosition;
                found = true;
            }
        }
        if (!found) { weapon.cooldownTimer = 0.08f; return; }
        const Vector2 direction = DirectionAtAngle(DirectionAngle(origin, target));
        Projectile spear{};
        spear.position = origin;
        spear.velocity = {direction.x * 930.0f * stats.projectileSpeedMultiplier,
                          direction.y * 930.0f * stats.projectileSpeedMultiplier};
        spear.damage = RollDamage(72.0f, stats, &spear.critical);
        spear.lifetime = 2.4f * stats.durationMultiplier;
        spear.radius = 18.0f * stats.areaMultiplier * stats.temporaryAreaMultiplier;
        spear.remainingPierces = 1000;
        spear.owner = ProjectileOwner::Player;
        spear.source = DamageSource::PlayerProjectile;
        spear.sourceWeapon = WeaponType::VoidLance;
        spear.color = Color{205, 70, 255, 255};
        projectiles.Spawn(spear);
        weapon.cooldownTimer = 1.38f * stats.cooldownMultiplier;
        return;
    }
    if (weapon.evolved == EvolvedWeaponType::TempestCannon) {
        if (weapon.cooldownTimer <= 0.0f)
            FireLinearWeapon(weapon, origin, stats, enemies, projectiles, 10.0f, true, bosses);
        return;
    }
    if (weapon.evolved != EvolvedWeaponType::StormArc || weapon.cooldownTimer > 0.0f) return;

    std::array<int, 5> hitEnemies{{-1, -1, -1, -1, -1}};
    int hitCount = 0;
    bool hitBoss = false;
    Vector2 current = origin;
    for (int jump = 0; jump < 5; ++jump) {
        enemies.QueryCircle(current, jump == 0 ? 1400.0f : 285.0f, queryScratch_);
        int selected = -1;
        float bestDistance = std::numeric_limits<float>::max();
        for (int rawIndex : queryScratch_) {
            bool alreadyHit = false;
            for (int seen = 0; seen < hitCount; ++seen) alreadyHit |= hitEnemies[static_cast<std::size_t>(seen)] == rawIndex;
            if (alreadyHit || !enemies.Items()[static_cast<std::size_t>(rawIndex)].active) continue;
            const Vector2 position = enemies.Items()[static_cast<std::size_t>(rawIndex)].position;
            const float dx = position.x - current.x;
            const float dy = position.y - current.y;
            const float distance = dx * dx + dy * dy;
            if (distance < bestDistance) { bestDistance = distance; selected = rawIndex; }
        }
        bool selectBoss = false;
        if (!hitBoss && bosses != nullptr && bosses->IsActive()) {
            const Vector2 position = bosses->ActiveBoss().position;
            const float dx = position.x - current.x;
            const float dy = position.y - current.y;
            const float distance = dx * dx + dy * dy;
            const float range = jump == 0 ? 1400.0f : 285.0f;
            if (distance <= range * range && distance < bestDistance) selectBoss = true;
        }
        if (selected < 0 && !selectBoss) break;
        const Vector2 next = selectBoss ? bosses->ActiveBoss().position
                                        : enemies.Items()[static_cast<std::size_t>(selected)].position;
        for (ChainSegment& segment : chainSegments_) if (segment.lifetime <= 0.0f) {
            segment = {current, next, 0.16f};
            break;
        }
        if (selectBoss) {
            bool critical = false;
            const float damage = RollDamage(38.0f, stats, &critical);
            bosses->Damage(damage, DamageSource::PlayerProjectile, critical);
            hitBoss = true;
        } else {
            const Vector2 direction = DirectionAtAngle(DirectionAngle(current, next));
            bool critical = false;
            const float damage = RollDamage(38.0f, stats, &critical);
            enemies.Damage(static_cast<std::size_t>(selected), damage, direction,
                           18.0f, DamageSource::PlayerProjectile, critical);
            hitEnemies[static_cast<std::size_t>(hitCount++)] = selected;
        }
        current = next;
    }
    weapon.cooldownTimer = 0.62f * stats.cooldownMultiplier;
}

void WeaponManager::FireSoulScythe(WeaponInstance& weapon, Player& player,
                                    EnemyManager& enemies, BossManager* bosses, bool evolved) {
    const WeaponStats base = GetWeaponStats(WeaponType::SoulScythe, weapon.level);
    const float areaScale = player.stats.areaMultiplier * player.stats.temporaryAreaMultiplier;
    const float radius = base.area * areaScale * (evolved ? 1.32f : 1.0f);
    const int cycle = static_cast<int>(weapon.phase);
    const bool fullCircle = evolved && cycle % 3 == 2;
    const int sweeps = fullCircle ? 1 : (evolved ? 2 : base.projectileCount);
    const float facingAngle = std::atan2(player.Facing().y, player.Facing().x);
    for (int sweepIndex = 0; sweepIndex < sweeps; ++sweepIndex) {
        const float centerAngle = facingAngle + ((cycle + sweepIndex) % 2 == 0 ? -0.45f : 0.45f) +
                                  (sweepIndex == 1 ? PI : 0.0f);
        const float arcRadians = fullCircle ? 2.0f * PI : base.orbitSpeed * DEG2RAD;
        enemies.QueryCircle(player.Position(), radius + 45.0f, queryScratch_);
        for (int rawIndex : queryScratch_) {
            Enemy& enemy = enemies.Items()[static_cast<std::size_t>(rawIndex)];
            if (!enemy.active || enemy.intangible) continue;
            const Vector2 delta{enemy.position.x - player.Position().x,
                                enemy.position.y - player.Position().y};
            float difference = std::atan2(delta.y, delta.x) - centerAngle;
            while (difference > PI) difference -= 2.0f * PI;
            while (difference < -PI) difference += 2.0f * PI;
            if (!fullCircle && std::fabs(difference) > arcRadians * 0.5f) continue;
            bool critical = false;
            float damage = RollDamage(base.damage * (evolved ? 1.55f : 1.0f), player.stats, &critical);
            enemies.Damage(static_cast<std::size_t>(rawIndex), damage, DirectionAtAngle(centerAngle),
                           base.knockback, DamageSource::PlayerArea, critical);
        }
        if (bosses != nullptr && bosses->IsActive()) {
            const Boss& boss = bosses->ActiveBoss();
            const Vector2 delta{boss.position.x - player.Position().x, boss.position.y - player.Position().y};
            float difference = std::atan2(delta.y, delta.x) - centerAngle;
            while (difference > PI) difference -= 2.0f * PI;
            while (difference < -PI) difference += 2.0f * PI;
            if (delta.x * delta.x + delta.y * delta.y <= (radius + boss.radius) * (radius + boss.radius) &&
                (fullCircle || std::fabs(difference) <= arcRadians * 0.5f)) {
                bool critical = false;
                float damage = RollDamage(base.damage * (evolved ? 1.55f : 1.0f), player.stats, &critical);
                bosses->Damage(damage, DamageSource::PlayerArea, critical);
            }
        }
        for (SweepVisual& visual : sweepVisuals_) if (visual.lifetime <= 0.0f) {
            const float centerDegrees = centerAngle * RAD2DEG;
            visual = {player.Position(), radius,
                      fullCircle ? 0.0f : centerDegrees - base.orbitSpeed * 0.5f,
                      fullCircle ? 360.0f : centerDegrees + base.orbitSpeed * 0.5f,
                      0.22f, evolved ? Color{210, 255, 205, 255} : Color{90, 225, 175, 255}};
            break;
        }
    }
    weapon.phase += 1.0f;
    weapon.cooldownTimer = base.cooldown * player.stats.cooldownMultiplier * (evolved ? 0.72f : 1.0f);
    if (audio_) audio_->Play(AudioCue::Spectral);
}

void WeaponManager::FireFrostShards(WeaponInstance& weapon, Vector2 origin,
                                     const PlayerStats& playerStats,
                                     const EnemyManager& enemies, ProjectileManager& projectiles,
                                     AreaEffectManager& areas, BossManager* bosses, bool evolved) {
    const Enemy* target = enemies.FindNearest(origin);
    Vector2 targetPosition = target != nullptr ? target->position : Vector2{};
    bool found = target != nullptr;
    if (bosses != nullptr && bosses->IsActive() &&
        (!found || DistanceSquared(origin, bosses->ActiveBoss().position) < DistanceSquared(origin, targetPosition))) {
        targetPosition = bosses->ActiveBoss().position; found = true;
    }
    if (!found) { weapon.cooldownTimer = 0.08f; return; }
    const WeaponStats stats = GetWeaponStats(WeaponType::FrostShards, weapon.level);
    const int count = std::min(10, stats.projectileCount + playerStats.projectileCountBonus + (evolved ? 2 : 0));
    const float center = DirectionAngle(origin, targetPosition);
    for (int index = 0; index < count; ++index) {
        const float fraction = count == 1 ? 0.5f : static_cast<float>(index) / (count - 1);
        const float angle = center + (fraction - 0.5f) * (evolved ? 0.58f : 0.40f);
        const Vector2 direction = DirectionAtAngle(angle);
        Projectile shard{};
        shard.position = origin;
        shard.velocity = {direction.x * stats.projectileSpeed * playerStats.projectileSpeedMultiplier,
                          direction.y * stats.projectileSpeed * playerStats.projectileSpeedMultiplier};
        shard.damage = RollDamage(stats.damage * (evolved ? 1.42f : 1.0f), playerStats, &shard.critical);
        shard.lifetime = stats.duration * playerStats.durationMultiplier;
        shard.radius = stats.area * (evolved ? 1.3f : 1.0f);
        shard.remainingPierces = stats.piercing + playerStats.globalPiercingBonus;
        shard.slowDuration = evolved ? 2.4f : 1.6f;
        shard.slowMultiplier = evolved ? 0.60f : 0.74f;
        shard.owner = ProjectileOwner::Player; shard.source = DamageSource::PlayerProjectile;
        shard.sourceWeapon = WeaponType::FrostShards; shard.color = Color{105, 225, 255, 255};
        projectiles.Spawn(shard);
    }
    if (evolved && static_cast<int>(weapon.phase) % 2 == 1) {
        AreaEffect nova{};
        nova.position = origin;
        nova.radius = 215.0f * playerStats.areaMultiplier * playerStats.temporaryAreaMultiplier;
        nova.damage = RollDamage(24.0f, playerStats, &nova.critical);
        nova.lifetime = nova.totalLifetime = 0.32f; nova.tickInterval = 1.0f; nova.ticksRemaining = 1;
        nova.slowDuration = 2.8f; nova.slowMultiplier = 0.58f;
        nova.source = DamageSource::PlayerArea; nova.sourceWeapon = WeaponType::FrostShards;
        nova.color = Color{130, 235, 255, 255};
        areas.Spawn(nova);
    }
    weapon.phase += 1.0f;
    weapon.cooldownTimer = stats.cooldown * playerStats.cooldownMultiplier * (evolved ? 0.72f : 1.0f);
    if (audio_) audio_->Play(AudioCue::ArcBolt);
}

void WeaponManager::FireBloodNeedles(WeaponInstance& weapon, Vector2 origin,
                                      const PlayerStats& stats, EnemyManager& enemies,
                                      ProjectileManager& projectiles, BossManager* bosses, bool evolved) {
    enemies.QueryCircle(origin, 1450.0f, queryScratch_);
    int validCount = 0;
    for (int raw : queryScratch_) if (enemies.Items()[static_cast<std::size_t>(raw)].active &&
                                      !enemies.Items()[static_cast<std::size_t>(raw)].intangible) ++validCount;
    if (validCount == 0 && (bosses == nullptr || !bosses->IsActive())) { weapon.cooldownTimer = 0.08f; return; }
    const WeaponStats weaponStats = GetWeaponStats(WeaponType::BloodNeedles, weapon.level);
    int count = weaponStats.projectileCount + stats.projectileCountBonus + (evolved ? 5 : 0);
    if (evolved && static_cast<int>(weapon.phase) % 4 == 3) count += 3;
    count = std::min(count, 14);
    int targetOrdinal = static_cast<int>(weapon.phase);
    for (int shotIndex = 0; shotIndex < count; ++shotIndex) {
        int targetIndex = -1;
        if (validCount > 0) {
            int desired = (targetOrdinal + shotIndex) % validCount;
            for (int raw : queryScratch_) {
                const Enemy& candidate = enemies.Items()[static_cast<std::size_t>(raw)];
                if (!candidate.active || candidate.intangible) continue;
                if (desired-- == 0) { targetIndex = raw; break; }
            }
        }
        Vector2 target = targetIndex >= 0 ? enemies.Items()[static_cast<std::size_t>(targetIndex)].position
                                          : bosses->ActiveBoss().position;
        const float angle = DirectionAngle(origin, target) + (shotIndex % 3 - 1) * 0.035f;
        const Vector2 direction = DirectionAtAngle(angle);
        Projectile needle{};
        needle.position = origin;
        needle.velocity = {direction.x * weaponStats.projectileSpeed * stats.projectileSpeedMultiplier,
                           direction.y * weaponStats.projectileSpeed * stats.projectileSpeedMultiplier};
        needle.damage = RollDamage(weaponStats.damage * (evolved ? 1.32f : 1.0f), stats, &needle.critical);
        needle.lifetime = weaponStats.duration * stats.durationMultiplier;
        needle.radius = weaponStats.area; needle.remainingPierces = stats.globalPiercingBonus;
        needle.homingTarget = targetIndex; needle.homingStrength = evolved ? 4.0f : 2.4f;
        needle.owner = ProjectileOwner::Player; needle.source = DamageSource::PlayerProjectile;
        needle.sourceWeapon = WeaponType::BloodNeedles; needle.color = Color{255, 58, 105, 255};
        projectiles.Spawn(needle);
    }
    weapon.phase += 1.0f;
    weapon.cooldownTimer = weaponStats.cooldown * stats.cooldownMultiplier * (evolved ? 0.68f : 1.0f);
    if (audio_) audio_->Play(AudioCue::Spectral);
}

void WeaponManager::SpawnGravityWell(WeaponInstance& weapon, Vector2 origin,
                                      const PlayerStats& stats, const EnemyManager& enemies,
                                      AreaEffectManager& areas, BossManager* bosses, bool evolved) {
    const Enemy* target = enemies.FindNearest(origin, 1300.0f);
    Vector2 position = target != nullptr ? target->position : origin;
    if (target == nullptr && bosses != nullptr && bosses->IsActive()) position = bosses->ActiveBoss().position;
    if (target == nullptr && (bosses == nullptr || !bosses->IsActive())) { weapon.cooldownTimer = 0.10f; return; }
    const WeaponStats weaponStats = GetWeaponStats(WeaponType::GravityWell, weapon.level);
    AreaEffect well{};
    well.position = position;
    well.radius = weaponStats.area * stats.areaMultiplier * stats.temporaryAreaMultiplier * (evolved ? 1.35f : 1.0f);
    well.damage = RollDamage(weaponStats.damage * (evolved ? 1.45f : 1.0f), stats, &well.critical);
    well.lifetime = well.totalLifetime = weaponStats.duration * stats.durationMultiplier * (evolved ? 1.25f : 1.0f);
    well.ticksRemaining = weaponStats.ticks + (evolved ? 4 : 0);
    well.tickInterval = well.lifetime / static_cast<float>(well.ticksRemaining);
    well.pullStrength = (evolved ? 155.0f : 92.0f) * stats.controlStrength;
    well.source = DamageSource::PlayerArea;
    well.sourceWeapon = WeaponType::GravityWell;
    well.color = evolved ? Color{220, 115, 255, 255} : Color{135, 75, 230, 255};
    if (areas.Spawn(well)) {
        weapon.cooldownTimer = weaponStats.cooldown * stats.cooldownMultiplier * (evolved ? 0.72f : 1.0f);
        if (audio_) audio_->Play(AudioCue::VoidLance);
    }
}

void WeaponManager::UpdateGuardianOrbs(WeaponInstance& weapon, float deltaTime, Vector2 origin,
                                       const PlayerStats& stats, EnemyManager& enemies,
                                       BossManager* bosses, bool damageEnabled) {
    const WeaponStats weaponStats = GetWeaponStats(weapon.type, weapon.level);
    weapon.phase += weaponStats.orbitSpeed * deltaTime;
    const bool celestial = weapon.evolved == EvolvedWeaponType::CelestialGuard;
    const int orbCount = celestial ? 6 : weaponStats.projectileCount;
    const float orbitRadius = (celestial ? 132.0f : (weapon.level >= 8 ? 112.0f : 94.0f)) * stats.areaMultiplier * stats.temporaryAreaMultiplier;
    const float orbRadius = weaponStats.area * stats.areaMultiplier * stats.temporaryAreaMultiplier * (celestial ? 1.2f : 1.0f);
    auto& enemyItems = enemies.Items();
    for (int orb = 0; orb < orbCount; ++orb) {
        const float angle = weapon.phase + (2.0f * PI) * static_cast<float>(orb) /
                                             static_cast<float>(orbCount);
        const Vector2 position{origin.x + std::cos(angle) * orbitRadius,
                               origin.y + std::sin(angle) * orbitRadius};
        if (!damageEnabled) continue;
        enemies.QueryCircle(position, orbRadius + 40.0f, queryScratch_);
        for (int rawIndex : queryScratch_) {
            const std::size_t enemyIndex = static_cast<std::size_t>(rawIndex);
            Enemy& enemy = enemyItems[enemyIndex];
            if (!enemy.active || enemy.orbitalHitCooldown > 0.0f ||
                !Overlaps(position, orbRadius, enemy.position, enemy.radius)) continue;
            const Vector2 direction{std::cos(angle), std::sin(angle)};
            bool critical = false;
            const float damage = RollDamage(weaponStats.damage * (celestial ? 1.65f : 1.0f),
                                            stats, &critical);
            enemies.Damage(enemyIndex, damage, direction, weaponStats.knockback,
                           DamageSource::PlayerOrbital, critical);
            enemy.orbitalHitCooldown = 0.4f;
            if (audio_) audio_->Play(AudioCue::Orbital);
        }
        if (bosses != nullptr && bosses->IsActive() &&
            bosses->ActiveBoss().orbitalHitCooldown <= 0.0f &&
            Overlaps(position, orbRadius, bosses->ActiveBoss().position, bosses->ActiveBoss().radius)) {
            bool critical = false;
            const float damage = RollDamage(weaponStats.damage * (celestial ? 1.65f : 1.0f),
                                            stats, &critical);
            bosses->Damage(damage, DamageSource::PlayerOrbital, critical);
            bosses->ActiveBoss().orbitalHitCooldown = 0.4f;
            if (audio_) audio_->Play(AudioCue::Orbital);
        }
    }
}

void WeaponManager::DrawOrbitals(Vector2 playerPosition, const PlayerStats& stats) const {
    for (const ChainSegment& segment : chainSegments_)
        if (segment.lifetime > 0.0f) DrawLineEx(segment.start, segment.end, 4.0f, SKYBLUE);
    for (const SweepVisual& sweep : sweepVisuals_) if (sweep.lifetime > 0.0f) {
        const float alpha = std::clamp(sweep.lifetime / 0.22f, 0.0f, 1.0f);
        DrawRing(sweep.center, sweep.radius - 18.0f, sweep.radius,
                 sweep.startAngle, sweep.endAngle, 36,
                 Color{sweep.color.r, sweep.color.g, sweep.color.b,
                       static_cast<unsigned char>(alpha * 210.0f)});
    }
    for (int index = 0; index < weaponCount_; ++index) {
        const WeaponInstance& weapon = weapons_[static_cast<std::size_t>(index)];
        if (weapon.type != WeaponType::GuardianOrbs) continue;
        const WeaponStats weaponStats = GetWeaponStats(weapon.type, weapon.level);
        const bool celestial = weapon.evolved == EvolvedWeaponType::CelestialGuard;
        const float orbitRadius = (celestial ? 132.0f : (weapon.level >= 8 ? 112.0f : 94.0f)) * stats.areaMultiplier * stats.temporaryAreaMultiplier;
        const float orbRadius = weaponStats.area * stats.areaMultiplier * stats.temporaryAreaMultiplier;
        const int count = celestial ? 6 : weaponStats.projectileCount;
        for (int orb = 0; orb < count; ++orb) {
            const float angle = weapon.phase + (2.0f * PI) * static_cast<float>(orb) /
                                                 static_cast<float>(count);
            const Vector2 position{playerPosition.x + std::cos(angle) * orbitRadius,
                                   playerPosition.y + std::sin(angle) * orbitRadius};
            DrawCircleV(position, orbRadius + 4.0f, Color{180, 120, 255, 65});
            DrawCircleV(position, celestial ? orbRadius * 1.2f : orbRadius,
                        celestial ? GOLD : GetWeaponDefinition(weapon.type).color);
        }
    }
}

bool WeaponManager::HasWeapon(WeaponType type) const { return WeaponLevel(type) > 0; }

int WeaponManager::WeaponLevel(WeaponType type) const {
    for (int index = 0; index < weaponCount_; ++index)
        if (weapons_[static_cast<std::size_t>(index)].type == type)
            return weapons_[static_cast<std::size_t>(index)].level;
    return 0;
}

bool WeaponManager::AddOrUpgradeWeapon(WeaponType type) {
    for (int index = 0; index < weaponCount_; ++index) {
        WeaponInstance& weapon = weapons_[static_cast<std::size_t>(index)];
        if (weapon.type != type) continue;
        if (weapon.level >= MaxWeaponLevel) return false;
        ++weapon.level;
        return true;
    }
    if (weaponCount_ >= weaponSlotLimit_) return false;
    weapons_[static_cast<std::size_t>(weaponCount_++)] = {type, 1, 0.15f, 0.0f};
    return true;
}

bool WeaponManager::ReplaceWeapon(int slot, WeaponType type) {
    if (slot < 0 || slot >= weaponCount_ || slot >= weaponSlotLimit_ || HasWeapon(type)) return false;
    weapons_[static_cast<std::size_t>(slot)] = {type, 1, 0.15f, 0.0f};
    return true;
}

bool WeaponManager::SwapWeaponSlots() {
    if (weaponCount_ < 2) return false;
    std::swap(weapons_[0], weapons_[1]);
    return true;
}

float WeaponManager::SlotCooldown(int slot) const {
    return slot >= 0 && slot < weaponCount_
               ? std::max(0.0f, weapons_[static_cast<std::size_t>(slot)].cooldownTimer) : 0.0f;
}

bool WeaponManager::HasPassive(PassiveType type) const { return PassiveLevel(type) > 0; }

int WeaponManager::PassiveLevel(PassiveType type) const {
    for (int index = 0; index < passiveCount_; ++index)
        if (passives_[static_cast<std::size_t>(index)].type == type)
            return passives_[static_cast<std::size_t>(index)].level;
    return 0;
}

bool WeaponManager::AddOrUpgradePassive(PassiveType type, PlayerStats& stats) {
    for (int index = 0; index < passiveCount_; ++index) {
        PassiveInstance& passive = passives_[static_cast<std::size_t>(index)];
        if (passive.type != type) continue;
        if (passive.level >= MaxPassiveLevel) return false;
        ++passive.level;
        ApplyPassiveLevel(type, stats);
        return true;
    }
    if (passiveCount_ >= MaxPassiveSlots) return false;
    passives_[static_cast<std::size_t>(passiveCount_++)] = {type, 1};
    ApplyPassiveLevel(type, stats);
    return true;
}

bool WeaponManager::IsEvolutionEligible(WeaponType type) const {
    return WeaponLevel(type) == MaxWeaponLevel && !IsEvolved(type) &&
           HasPassive(GetEvolutionDefinition(type).requiredPassive);
}

bool WeaponManager::EvolveWeapon(WeaponType type) {
    if (!IsEvolutionEligible(type)) return false;
    for (int index = 0; index < weaponCount_; ++index) {
        WeaponInstance& weapon = weapons_[static_cast<std::size_t>(index)];
        if (weapon.type != type) continue;
        weapon.evolved = GetEvolutionDefinition(type).evolvedWeapon;
        weapon.cooldownTimer = 0.0f;
        return true;
    }
    return false;
}

bool WeaponManager::IsEvolved(WeaponType type) const {
    return Evolution(type) != EvolvedWeaponType::None;
}

EvolvedWeaponType WeaponManager::Evolution(WeaponType type) const {
    for (int index = 0; index < weaponCount_; ++index)
        if (weapons_[static_cast<std::size_t>(index)].type == type)
            return weapons_[static_cast<std::size_t>(index)].evolved;
    return EvolvedWeaponType::None;
}

int WeaponManager::EvolutionCount() const {
    int count = 0;
    for (int index = 0; index < weaponCount_; ++index)
        if (weapons_[static_cast<std::size_t>(index)].evolved != EvolvedWeaponType::None) ++count;
    return count;
}

int WeaponManager::EligibleEvolutionCount() const {
    int count = 0;
    for (int raw = 0; raw < static_cast<int>(WeaponType::Count); ++raw)
        if (IsEvolutionEligible(static_cast<WeaponType>(raw))) ++count;
    return count;
}

float WeaponManager::RollDamage(float baseDamage, const PlayerStats& stats, bool* criticalResult) const {
    const bool critical = GetRandomValue(0, 9999) <
                          static_cast<int>(std::clamp(stats.criticalChance, 0.0f, 1.0f) * 10000.0f);
    if (criticalResult != nullptr) *criticalResult = critical;
    return baseDamage * stats.damageMultiplier * (critical ? stats.criticalMultiplier : 1.0f);
}

void WeaponManager::ApplyPassiveLevel(PassiveType type, PlayerStats& stats) {
    switch (type) {
        case PassiveType::SwiftBoots: stats.moveSpeed += Balance::PlayerMoveSpeed * 0.05f; break;
        case PassiveType::PowerCore: stats.damageMultiplier += 0.10f; break;
        case PassiveType::ChronoGear:
            stats.cooldownMultiplier = std::max(0.35f, stats.cooldownMultiplier * 0.93f); break;
        case PassiveType::ExpansionRune: stats.areaMultiplier += 0.10f; break;
        case PassiveType::MagnetStone: stats.magnetRadius += 30.0f; break;
        case PassiveType::VitalHeart:
            stats.maxHP += 15.0f;
            stats.currentHP += 15.0f;
            stats.hpRegeneration += 0.20f;
            break;
        case PassiveType::IronShell: stats.armor += 1.0f; break;
        case PassiveType::LuckyCharm:
            stats.luck += 0.20f;
            stats.criticalChance = std::min(0.50f, stats.criticalChance + 0.01f);
            break;
        case PassiveType::FocusCrystal:
            stats.projectileSpeedMultiplier += 0.10f;
            stats.criticalChance = std::min(0.50f, stats.criticalChance + 0.01f);
            break;
        case PassiveType::EmberCore:
            stats.areaMultiplier += 0.06f;
            stats.durationMultiplier += 0.06f;
            break;
        case PassiveType::OrbitalEngine:
            stats.areaMultiplier += 0.05f;
            stats.durationMultiplier += 0.08f;
            break;
        case PassiveType::WindSigil:
            stats.moveSpeed += Balance::PlayerMoveSpeed * 0.03f;
            stats.projectileSpeedMultiplier += 0.06f;
            break;
        case PassiveType::PiercingEye:
            stats.criticalChance = std::min(0.50f, stats.criticalChance + 0.015f);
            ++stats.globalPiercingBonus;
            break;
        case PassiveType::TitanCore:
            stats.damageMultiplier += 0.05f;
            stats.maxHP += 8.0f;
            stats.currentHP += 8.0f;
            break;
        case PassiveType::ExecutionerSigil:
            stats.lowHealthDamageBonus += 0.06f;
            break;
        case PassiveType::FrozenHeart:
            stats.slowEffectiveness += 0.08f;
            stats.armor += 0.20f;
            break;
        case PassiveType::SoulHarvest:
            stats.xpMultiplier += 0.04f;
            break;
        case PassiveType::BloodPact:
            stats.damageMultiplier += 0.08f;
            stats.maxHP = std::max(20.0f, stats.maxHP - 4.0f);
            stats.currentHP = std::min(stats.currentHP, stats.maxHP);
            break;
        case PassiveType::GraviticCore:
            stats.durationMultiplier += 0.06f;
            stats.areaMultiplier += 0.03f;
            stats.controlStrength += 0.10f;
            break;
        case PassiveType::SoulChain:
            if (PassiveLevel(type) == 2 || PassiveLevel(type) == 4)
                stats.projectileCountBonus = std::min(2, stats.projectileCountBonus + 1);
            break;
        case PassiveType::Count: break;
    }
}
