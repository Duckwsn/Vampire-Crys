#include "gameplay/GameSession.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "gameplay/Balance.h"
#include "game_modes/WorldNavigation.h"
#include "ui/VisualStyle.h"

namespace {
bool CirclesOverlap(Vector2 a, float aRadius, Vector2 b, float bRadius) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float radius = aRadius + bRadius;
    return dx * dx + dy * dy <= radius * radius;
}

Vector2 Normalized(Vector2 value) {
    const float lengthSquared = value.x * value.x + value.y * value.y;
    if (lengthSquared <= 0.0001f) return {};
    const float inverse = 1.0f / std::sqrt(lengthSquared);
    return {value.x * inverse, value.y * inverse};
}
} // namespace

void GameSession::ResetSharedRunSystems() {
    navigation_ = nullptr;
    carriedEnemyDamage_ = carriedBossDamage_ = 0.0;
    player_.Reset(selectedCharacter_);
    player_.stats.maxHP = std::max(20.0f, player_.stats.maxHP * runModifiers_.playerHp);
    player_.stats.currentHP = player_.stats.maxHP;
    player_.stats.damageMultiplier *= runModifiers_.playerDamage;
    player_.stats.moveSpeed *= runModifiers_.playerSpeed;
    player_.SanitizeStats();
    enemies_.Reset();
    projectiles_.Reset();
    areas_.Reset();
    xpOrbs_.Reset();
    weapons_.Reset(GetCharacterDefinition(selectedCharacter_).startingWeapon);
    weapons_.SetCharacter(selectedCharacter_);
    bosses_.Reset();
    bosses_.SetSpawnMultipliers(runModifiers_.bossHp, runModifiers_.bossDamage);
    chests_.Reset();
    pickups_.Reset();
    particles_.Reset();
    floatingTexts_.Reset();
    screenShake_.Reset();
    statistics_ = {};
    elapsedTime_ = 0.0f;
    kills_ = 0;
    pendingLevelUps_ = 0;
    choiceCount_ = 0;
    choices_ = {};
    chestChoices_ = {};
    chestChoiceCount_ = 0;
    pendingChestReward_ = false;
    pendingWeaponReplacement_ = false;
    replacementFromLevelUp_ = false;
    manualCombat_ = false;
    attackRequest_ = {};
    bossDeathThisFrame_ = {};
    bossDeathThisFrameValid_ = false;
    minibossDeathThisFrame_ = false;
    minibossDeathTypeThisFrame_ = EnemyType::GraveWarden;
    enemySpeedMultiplier_ = 1.0f;
    endlessCyclesCompleted_ = 0;
    if (queryScratch_.capacity() < 600) queryScratch_.reserve(600);

    updateMilliseconds_ = 0.0f;
    drawMilliseconds_ = 0.0f;
    playerTrailTimer_ = 0.0f;
}

void GameSession::Reset() { ResetSharedRunSystems(); }

void GameSession::Update(float deltaTime) {
    bossDeathThisFrameValid_ = false;
    minibossDeathThisFrame_ = false;
    player_.Update(deltaTime, Balance::Arena, navigation_);
    if (player_.ConsumeDashStarted()) {
        particles_.SpawnBurst(player_.Position(), VisualStyle::EnergyColor(player_.VisualTime()),
                              28, 230.0f, ParticleKind::Shard, ParticlePriority::Important);
        screenShake_.Add(2.0f, 0.12f);
    }
    playerTrailTimer_ -= deltaTime;
    if (playerTrailTimer_ <= 0.0f) {
        const Color energy = VisualStyle::EnergyColor(player_.VisualTime(), 0.72f, 1.0f);
        const Vector2 trailPosition{player_.Position().x - player_.Facing().x * player_.Radius() * 0.65f,
                                    player_.Position().y - player_.Facing().y * player_.Radius() * 0.65f};
        particles_.SpawnBurst(trailPosition, energy, player_.IsMoving() ? 3 : 1,
                              player_.IsMoving() ? 65.0f : 25.0f);
        playerTrailTimer_ = player_.IsMoving() ? 0.035f : 0.075f;
    }
    bosses_.Update(deltaTime, player_, projectiles_, navigation_);
    statistics_.damageTaken += bosses_.DamageToPlayerThisFrame();
    if (bosses_.DamageToPlayerThisFrame() > 0.0f) screenShake_.Add(4.5f, 0.22f);
    enemies_.Update(deltaTime, player_.Position(), projectiles_, enemySpeedMultiplier_, navigation_);
    const float hunterBonus = player_.CharacterDefinitionData().trait == CharacterTrait::MarkedPrey
                                  ? 0.15f : 0.0f;
    enemies_.SetPlayerDamageBonuses(hunterBonus, player_.stats.lowHealthDamageBonus);
    bosses_.SetPlayerDamageBonuses(hunterBonus, player_.stats.lowHealthDamageBonus);
    if (manualCombat_)
        weapons_.UpdateManual(deltaTime, attackRequest_, player_, enemies_, projectiles_, areas_, &bosses_);
    else
        weapons_.Update(deltaTime, player_, enemies_, projectiles_, areas_, &bosses_);
    for (Projectile& projectile : projectiles_.Items()) {
        if (!projectile.active || projectile.owner != ProjectileOwner::Player ||
            projectile.homingStrength <= 0.0f || projectile.homingTarget < 0 ||
            projectile.homingTarget >= static_cast<int>(enemies_.Items().size())) continue;
        const Enemy& target = enemies_.Items()[static_cast<std::size_t>(projectile.homingTarget)];
        if (!target.active || target.intangible) continue;
        const float speed = std::sqrt(projectile.velocity.x * projectile.velocity.x +
                                      projectile.velocity.y * projectile.velocity.y);
        const Vector2 desired = Normalized({target.position.x - projectile.position.x,
                                            target.position.y - projectile.position.y});
        const float blend = std::clamp(projectile.homingStrength * deltaTime, 0.0f, 0.20f);
        const Vector2 current = Normalized(projectile.velocity);
        const Vector2 steered = Normalized({current.x + (desired.x - current.x) * blend,
                                            current.y + (desired.y - current.y) * blend});
        projectile.velocity = {steered.x * speed, steered.y * speed};
    }
    projectiles_.Update(deltaTime);
    int trailBudget = 40;
    for (Projectile& projectile : projectiles_.Items()) {
        if (!projectile.active || projectile.owner == ProjectileOwner::Enemy || trailBudget <= 0) continue;
        projectile.trailTimer -= deltaTime;
        if (projectile.trailTimer > 0.0f) continue;
        const bool major = projectile.sourceWeapon == WeaponType::VoidLance ||
                           projectile.sourceWeapon == WeaponType::ThunderCannon;
        particles_.SpawnBurst(projectile.position, projectile.color, major ? 2 : 1,
                              major ? 40.0f : 22.0f, major ? ParticleKind::Shard : ParticleKind::Square,
                              ParticlePriority::Low);
        projectile.trailTimer = major ? 0.035f : 0.075f;
        --trailBudget;
    }
    ResolveProjectileCollisions();
    ResolveExplosionEvents();
    areas_.Update(deltaTime, player_.Position(), enemies_, &bosses_);
    if (navigation_) enemies_.ConstrainToNavigation(*navigation_);
    player_.RegisterAoEHits(enemies_.AoEHitsThisFrame());
    ResolveEnemyPlayerCollisions();
    ResolveEnemyBlastEvents();
    ConsumeEnemyDeaths();
    ConsumeBossDeath();
    ConsumeDamageFeedback();
    ResolvePickups(deltaTime);
    if (!pendingChestReward_ && chests_.Update(player_.Position(), player_.Radius())) {
        ++statistics_.chestsOpened;
        pendingChestReward_ = true;
        PrepareChestRewards();
        particles_.SpawnBurst(player_.Position(), GOLD, 18, 170.0f);
        if (audio_) audio_->Play(AudioCue::Chest);
    }
    particles_.Update(deltaTime);
    floatingTexts_.Update(deltaTime);
    screenShake_.Update(deltaTime);

    const int collectedXP = xpOrbs_.Update(deltaTime, player_.Position(), player_.Radius(),
                                            player_.stats.magnetRadius);
    if (collectedXP > 0) {
        AddXP(collectedXP); statistics_.xpCollected += collectedXP;
        if (audio_) audio_->Play(AudioCue::XPCollect);
    }
    statistics_.timeSurvived = elapsedTime_;
    statistics_.levelReached = player_.stats.level;
    statistics_.enemiesKilled = kills_;
    statistics_.damageToBosses = carriedBossDamage_ + bosses_.DamageTaken();
    statistics_.damageDealt = carriedEnemyDamage_ + enemies_.DamageDealt() +
                              carriedBossDamage_ + bosses_.DamageTaken();
    statistics_.evolutionsObtained = weapons_.EvolutionCount();
}

void GameSession::DrawWorld() const {
    if (navigation_) navigation_->DrawBackground();
    else VisualStyle::DrawEnvironment(player_.Position(), Balance::Arena, player_.VisualTime());

    areas_.Draw();
    bosses_.Draw();
    chests_.Draw();
    pickups_.Draw();
    xpOrbs_.Draw();
    enemies_.Draw();
    player_.Draw();
    weapons_.DrawOrbitals(player_.Position(), player_.stats);
    projectiles_.Draw();
    particles_.Draw();
    floatingTexts_.Draw();
    if (navigation_) navigation_->DrawForeground(player_.Position());
}

void GameSession::DrawCollisionDebug() const {
    if (navigation_) navigation_->DrawDebug();
    DrawCircleLines(static_cast<int>(player_.Position().x), static_cast<int>(player_.Position().y),
                    player_.Radius(), LIME);
    DrawCircleLines(static_cast<int>(player_.Position().x), static_cast<int>(player_.Position().y),
                    player_.stats.magnetRadius, Color{60, 220, 170, 90});
    for (const Enemy& enemy : enemies_.Items()) if (enemy.active)
        DrawCircleLines(static_cast<int>(enemy.position.x), static_cast<int>(enemy.position.y), enemy.radius, RED);
    for (const Projectile& projectile : projectiles_.Items()) if (projectile.active)
        DrawCircleLines(static_cast<int>(projectile.position.x), static_cast<int>(projectile.position.y), projectile.radius, YELLOW);
    if (bosses_.IsActive()) {
        const Boss& boss = bosses_.ActiveBoss();
        DrawCircleLines(static_cast<int>(boss.position.x), static_cast<int>(boss.position.y), boss.radius, MAGENTA);
    }
}

Vector2 GameSession::ConstrainCamera(Vector2 target, Vector2 viewportHalfSize) const {
    return navigation_ ? navigation_->ConstrainCamera(target, viewportHalfSize) : target;
}

void GameSession::SetPlayerPosition(Vector2 position) {
    player_.SetPosition(navigation_ ? navigation_->FindNearestWalkablePosition(position, player_.Radius())
                                    : position);
}

void GameSession::ResetStageCombatSystems() {
    carriedEnemyDamage_ += enemies_.DamageDealt();
    carriedBossDamage_ += bosses_.DamageTaken();
    enemies_.Reset();
    projectiles_.Reset();
    areas_.Reset();
    xpOrbs_.Reset();
    bosses_.Reset();
    bosses_.SetSpawnMultipliers(runModifiers_.bossHp, runModifiers_.bossDamage);
    bosses_.SetNavigation(navigation_);
    chests_.Reset();
    pickups_.Reset();
    particles_.Reset();
    floatingTexts_.Reset();
    screenShake_.Reset();
    pendingChestReward_ = false;
    chestChoiceCount_ = 0;
    bossDeathThisFrameValid_ = false;
    minibossDeathThisFrame_ = false;
}

void GameSession::ResolveProjectileCollisions() {
    auto& projectiles = projectiles_.Items();
    auto& enemies = enemies_.Items();
    for (Projectile& projectile : projectiles) {
        if (!projectile.active) continue;
        const float playerDx = projectile.position.x - player_.Position().x;
        const float playerDy = projectile.position.y - player_.Position().y;
        if (playerDx * playerDx + playerDy * playerDy > 1700.0f * 1700.0f) {
            projectiles_.Deactivate(projectile, projectile.explosive);
            continue;
        }
        if (projectile.owner != ProjectileOwner::Player) {
            if (CirclesOverlap(projectile.position, projectile.radius,
                               player_.Position(), player_.Radius())) {
                if (player_.TakeDamage(projectile.damage)) {
                    statistics_.damageTaken += player_.LastDamageTaken(); screenShake_.Add(3.5f);
                    if (audio_) audio_->Play(AudioCue::PlayerHit);
                }
                projectiles_.Deactivate(projectile);
            }
            continue;
        }

        if (bosses_.IsActive() && !projectile.hitBoss &&
            CirclesOverlap(projectile.position, projectile.radius,
                           bosses_.ActiveBoss().position, bosses_.ActiveBoss().radius)) {
            if (projectile.slowDuration > 0.0f)
                bosses_.ApplySlow(projectile.slowDuration,
                    1.0f - (1.0f - projectile.slowMultiplier) * player_.stats.slowEffectiveness);
            if (projectile.explosive) {
                projectiles_.Deactivate(projectile, true);
                continue;
            }
            bosses_.Damage(projectile.damage, projectile.source, projectile.critical);
            projectile.hitBoss = true;
            if (projectile.remainingPierces > 0) --projectile.remainingPierces;
            else {
                projectiles_.Deactivate(projectile);
                continue;
            }
        }

        enemies_.QueryCircle(projectile.position, projectile.radius + 40.0f, queryScratch_);
        for (int rawIndex : queryScratch_) {
            const std::size_t enemyIndex = static_cast<std::size_t>(rawIndex);
            Enemy& enemy = enemies[enemyIndex];
            if (!enemy.active || enemy.intangible || !CirclesOverlap(projectile.position, projectile.radius,
                                                  enemy.position, enemy.radius)) continue;
            if (projectile.lastHitEnemy == static_cast<int>(enemyIndex) &&
                projectile.repeatHitCooldown > 0.0f) continue;
            if (projectile.explosive) {
                projectiles_.Deactivate(projectile, true);
                break;
            }
            const Vector2 direction = Normalized(projectile.velocity);
            if (projectile.slowDuration > 0.0f)
                enemies_.ApplySlow(enemyIndex, projectile.slowDuration,
                                   1.0f - (1.0f - projectile.slowMultiplier) *
                                              player_.stats.slowEffectiveness);
            enemies_.Damage(enemyIndex, projectile.damage, direction, projectile.knockback,
                            projectile.source, projectile.critical);
            projectile.lastHitEnemy = static_cast<int>(enemyIndex);
            projectile.repeatHitCooldown = 0.10f;
            if (projectile.remainingPierces > 0) --projectile.remainingPierces;
            else {
                projectiles_.Deactivate(projectile);
                break;
            }
        }
    }
}

void GameSession::ResolveExplosionEvents() {
    for (int index = 0; index < projectiles_.ExplosionEventCount(); ++index) {
        const ExplosionEvent& explosion =
            projectiles_.ExplosionEvents()[static_cast<std::size_t>(index)];
        AreaEffect area{};
        area.position = explosion.position;
        area.radius = explosion.radius;
        area.damage = explosion.damage;
        area.lifetime = 0.28f;
        area.totalLifetime = area.lifetime;
        area.tickInterval = 1.0f;
        area.ticksRemaining = 1;
        area.knockback = explosion.knockback;
        area.source = explosion.source;
        area.color = explosion.color;
        area.critical = explosion.critical;
        areas_.Spawn(area);
        particles_.SpawnBurst(explosion.position, explosion.color,
                              explosion.radius >= 100.0f ? 28 : 12,
                              explosion.radius >= 100.0f ? 190.0f : 110.0f,
                              ParticleKind::Shard,
                              explosion.radius >= 100.0f ? ParticlePriority::Important : ParticlePriority::Normal);
        if (explosion.radius >= 100.0f) screenShake_.Add(3.0f, 0.16f);
        if (weapons_.IsEvolved(WeaponType::ThunderCannon)) {
            enemies_.QueryCircle(explosion.position, explosion.radius + 360.0f, queryScratch_);
            std::array<int, 3> selected{{-1, -1, -1}};
            int selectedCount = 0;
            for (int rawIndex : queryScratch_) {
                if (selectedCount >= 3 || !enemies_.Items()[static_cast<std::size_t>(rawIndex)].active)
                    continue;
                bool duplicate = false;
                for (int seen = 0; seen < selectedCount; ++seen)
                    duplicate |= selected[static_cast<std::size_t>(seen)] == rawIndex;
                if (duplicate) continue;
                selected[static_cast<std::size_t>(selectedCount++)] = rawIndex;
                AreaEffect lightning{};
                lightning.position = enemies_.Items()[static_cast<std::size_t>(rawIndex)].position;
                lightning.radius = explosion.radius * 0.58f;
                lightning.damage = explosion.damage * 0.52f;
                lightning.lifetime = 0.24f;
                lightning.totalLifetime = lightning.lifetime;
                lightning.tickInterval = 1.0f;
                lightning.ticksRemaining = 1;
                lightning.source = DamageSource::PlayerArea;
                lightning.color = SKYBLUE;
                lightning.critical = explosion.critical;
                areas_.Spawn(lightning);
            }
        }
    }
    projectiles_.ClearExplosionEvents();
}

void GameSession::ResolveEnemyPlayerCollisions() {
    for (const Enemy& enemy : enemies_.Items()) {
        if (enemy.active && enemies_.CanContactPlayer(enemy) && CirclesOverlap(enemy.position, enemy.radius,
                                           player_.Position(), player_.Radius()))
            if (player_.TakeDamage(enemy.contactDamage)) {
                statistics_.damageTaken += player_.LastDamageTaken(); screenShake_.Add(3.0f);
                if (audio_) audio_->Play(AudioCue::PlayerHit);
            }
    }
}

void GameSession::ResolveEnemyBlastEvents() {
    for (int index = 0; index < enemies_.BlastEventCount(); ++index) {
        const EnemyBlastEvent& blast = enemies_.BlastEvents()[static_cast<std::size_t>(index)];
        if (CirclesOverlap(blast.position, blast.radius, player_.Position(), player_.Radius()))
            if (player_.TakeDamage(blast.damage)) {
                statistics_.damageTaken += player_.LastDamageTaken(); screenShake_.Add(6.0f, 0.28f);
                if (audio_) audio_->Play(AudioCue::PlayerHit);
            }
        AreaEffect visual{};
        visual.position = blast.position;
        visual.radius = blast.radius;
        visual.lifetime = 0.35f;
        visual.totalLifetime = visual.lifetime;
        visual.ticksRemaining = 0;
        visual.color = Color{255, 72, 48, 255};
        areas_.Spawn(visual);
    }
    enemies_.ClearBlastEvents();
}

void GameSession::ConsumeEnemyDeaths() {
    for (int index = 0; index < enemies_.DeathEventCount(); ++index) {
        const EnemyDeathEvent& death = enemies_.DeathEvents()[static_cast<std::size_t>(index)];
        xpOrbs_.Spawn(death.position, std::max(1, static_cast<int>(
            std::round(death.xpReward * player_.stats.xpMultiplier))));
        ++kills_;
        if (death.spawnSource != SpawnSource::Necromancer) ++statistics_.scoreEligibleKills;
        if (death.elite) ++statistics_.elitesKilled;
        particles_.SpawnBurst(death.position, death.elite ? GOLD : Color{150, 215, 125, 255},
                              death.elite ? 14 : 5, death.elite ? 150.0f : 80.0f);
        PickupType drop{};
        if (death.miniboss || death.spawnSource == SpawnSource::EliteHunt) {
            const bool healthAllowed = runModifiers_.healthDropChance >= 0.5f;
            pickups_.Spawn(death.position, death.type == EnemyType::GraveWarden && healthAllowed
                                               ? PickupType::Health : PickupType::Magnet);
        } else if (death.spawnSource != SpawnSource::Necromancer &&
                   loot_.Roll(player_.stats.luck, death.elite, drop,
                              runModifiers_.healthDropChance)) {
            pickups_.Spawn(death.position, drop);
        }
        if (death.miniboss) {
            ++statistics_.minibossesKilled;
            minibossDeathThisFrame_ = true;
            minibossDeathTypeThisFrame_ = death.type;
            projectiles_.ClearOwner(ProjectileOwner::Enemy);
            particles_.SpawnBurst(death.position, GOLD, 45, 220.0f,
                                  ParticleKind::Shard, ParticlePriority::Important);
        }
        if (audio_) audio_->Play(AudioCue::EnemyDeath);
    }
    enemies_.ClearDeathEvents();
}

void GameSession::ConsumeBossDeath() {
    if (!bosses_.HasDeathEvent()) return;
    const BossDeathEvent death = bosses_.ConsumeDeathEvent();
    bossDeathThisFrame_ = death;
    bossDeathThisFrameValid_ = true;
    projectiles_.ClearOwner(ProjectileOwner::Boss);
    ++statistics_.bossesKilled;
    particles_.SpawnBurst(death.position, death.finalBoss ? VIOLET : GOLD, 90, 260.0f,
                          ParticleKind::Shard, ParticlePriority::Important);
    screenShake_.Add(death.finalBoss ? 11.0f : 8.0f, 0.55f);
    if (audio_) audio_->Play(death.finalBoss ? AudioCue::Victory : AudioCue::EnemyDeath);
}

void GameSession::AddXP(int amount) {
    player_.stats.currentXP += amount;
    while (player_.stats.currentXP >= player_.stats.xpRequired) {
        player_.stats.currentXP -= player_.stats.xpRequired;
        ++player_.stats.level;
        ++pendingLevelUps_;
        if (audio_) audio_->Play(AudioCue::LevelUp);
        player_.stats.xpRequired = Balance::XPRequiredForLevel(player_.stats.level);
    }
}

void GameSession::GrantDebugXP(int amount) { AddXP(amount); }

void GameSession::DebugKillAll() {
    auto& items = enemies_.Items();
    for (std::size_t index = 0; index < items.size(); ++index) if (items[index].active)
        enemies_.Damage(index, items[index].hp + 1.0f, {}, 0.0f, DamageSource::PlayerArea);
    ConsumeEnemyDeaths();
    enemies_.RebuildGrid();
}

void GameSession::DebugSpawnBoss(BossType type) {
    bosses_.SpawnDebug(type, player_.Position());
}

void GameSession::DebugDamageBoss(float healthFraction) {
    if (bosses_.IsActive())
        bosses_.Damage(bosses_.ActiveBoss().maxHP * healthFraction, DamageSource::PlayerArea);
}

void GameSession::DebugPrepareEvolution(WeaponType type) {
    while (weapons_.WeaponLevel(type) < WeaponManager::MaxWeaponLevel)
        if (!weapons_.AddOrUpgradeWeapon(type)) break;
    const PassiveType required = GetEvolutionDefinition(type).requiredPassive;
    if (!weapons_.HasPassive(required)) weapons_.AddOrUpgradePassive(required, player_.stats);
    DebugSpawnChest();
}

void GameSession::DebugSpawnChest() {
    chests_.Spawn(player_.Position());
}

void GameSession::ClearBossHazards() { bosses_.ClearHazards(projectiles_); }

void GameSession::AdvanceRunTime(float seconds, float maximum) {
    elapsedTime_ = std::clamp(elapsedTime_ + seconds, 0.0f, maximum);
}

ScoreBreakdown GameSession::CurrentScore() const {
    return CalculateSurvivalScore(statistics_.scoreEligibleKills, statistics_.elitesKilled,
                                  statistics_.minibossesKilled, statistics_.bossesKilled,
                                  elapsedTime_, survivalConfig_);
}

void GameSession::GrantEvolutionOpportunity(Vector2 position) {
    chests_.Spawn(position);
}

void GameSession::ResolvePickups(float deltaTime) {
    const PickupResult result = pickups_.Update(deltaTime, player_.Position(), player_.Radius());
    if (result.health > 0) player_.Heal(player_.stats.maxHP * 0.20f * result.health);
    if (result.magnet > 0) xpOrbs_.AttractAll();
    if (result.bomb > 0) {
        enemies_.QueryCircle(player_.Position(), 850.0f, queryScratch_);
        auto& items = enemies_.Items();
        for (int rawIndex : queryScratch_) {
            const std::size_t index = static_cast<std::size_t>(rawIndex);
            if (!items[index].active) continue;
            const Vector2 direction = Normalized({items[index].position.x - player_.Position().x,
                                                  items[index].position.y - player_.Position().y});
            enemies_.Damage(index, 120.0f, direction, 90.0f, DamageSource::PlayerArea);
        }
        ConsumeEnemyDeaths();
    }
    const int collected = result.health + result.magnet + result.bomb;
    if (collected > 0) {
        statistics_.pickupsCollected += collected;
        particles_.SpawnBurst(player_.Position(), SKYBLUE, 10, 130.0f);
        if (audio_) audio_->Play(AudioCue::Pickup);
    }
}

void GameSession::ConsumeDamageFeedback() {
    for (int index = 0; index < enemies_.DamageFeedbackEventCount(); ++index) {
        const DamageFeedbackEvent& feedback =
            enemies_.DamageFeedbackEvents()[static_cast<std::size_t>(index)];
        floatingTexts_.Spawn(feedback.position, feedback.amount, feedback.critical);
        if (audio_) audio_->Play(AudioCue::EnemyHit);
        particles_.SpawnBurst(feedback.position, feedback.critical ? GOLD : RAYWHITE,
                              feedback.critical ? 5 : 2, feedback.critical ? 150.0f : 90.0f,
                              ParticleKind::Shard, ParticlePriority::Low);
    }
    enemies_.ClearDamageFeedbackEvents();
    for (int index = 0; index < bosses_.DamageFeedbackEventCount(); ++index) {
        const DamageFeedbackEvent& feedback =
            bosses_.DamageFeedbackEvents()[static_cast<std::size_t>(index)];
        floatingTexts_.Spawn(feedback.position, feedback.amount, feedback.critical);
        particles_.SpawnBurst(feedback.position, feedback.critical ? GOLD : VIOLET,
                              feedback.critical ? 8 : 3, 135.0f, ParticleKind::Shard,
                              ParticlePriority::Normal);
    }
    bosses_.ClearDamageFeedbackEvents();
}

void GameSession::PrepareUpgradeChoices() {
    std::vector<UpgradeChoice> candidates;
    candidates.reserve(static_cast<std::size_t>(WeaponType::Count) +
                       static_cast<std::size_t>(PassiveType::Count));

    for (int raw = 0; raw < static_cast<int>(WeaponType::Count); ++raw) {
        const WeaponType type = static_cast<WeaponType>(raw);
        const int level = weapons_.WeaponLevel(type);
        if ((level == 0 && weapons_.WeaponCount() >= weapons_.WeaponSlotLimit() && !manualCombat_) ||
            level >= WeaponManager::MaxWeaponLevel) continue;
        const WeaponDefinition& definition = GetWeaponDefinition(type);
        candidates.push_back({UpgradeKind::Weapon, type, PassiveType::SwiftBoots,
                              definition.rarity, definition.name,
                              level == 0 ? "New Weapon" : "Weapon Upgrade",
                              definition.description, GetWeaponLevelEffect(type, level + 1),
                              level, level + 1});
    }

    for (int raw = 0; raw < static_cast<int>(PassiveType::Count); ++raw) {
        const PassiveType type = static_cast<PassiveType>(raw);
        const int level = weapons_.PassiveLevel(type);
        if ((level == 0 && weapons_.PassiveCount() >= WeaponManager::MaxPassiveSlots) ||
            level >= WeaponManager::MaxPassiveLevel) continue;
        const PassiveDefinition& definition = GetPassiveDefinition(type);
        candidates.push_back({UpgradeKind::Passive, WeaponType::ArcBolt, type,
                              definition.rarity, definition.name,
                              level == 0 ? "New Passive" : "Passive Upgrade",
                              definition.description, GetPassiveLevelEffect(type, level + 1),
                              level, level + 1});
    }

    choiceCount_ = 0;
    while (choiceCount_ < 3 && !candidates.empty()) {
        float totalWeight = 0.0f;
        for (const UpgradeChoice& candidate : candidates)
            totalWeight += RarityWeight(candidate.rarity, player_.stats.luck);
        const float rollUnit = static_cast<float>(GetRandomValue(0, 1000000)) / 1000000.0f;
        float roll = rollUnit * totalWeight;
        std::size_t selected = 0;
        for (; selected + 1 < candidates.size(); ++selected) {
            roll -= RarityWeight(candidates[selected].rarity, player_.stats.luck);
            if (roll <= 0.0f) break;
        }
        choices_[static_cast<std::size_t>(choiceCount_++)] = candidates[selected];
        candidates.erase(candidates.begin() + static_cast<std::ptrdiff_t>(selected));
    }
    if (choiceCount_ == 0) pendingLevelUps_ = 0;
}

void GameSession::SelectUpgrade(int index) {
    if (index < 0 || index >= choiceCount_ || pendingLevelUps_ <= 0) return;
    const UpgradeChoice& choice = choices_[static_cast<std::size_t>(index)];
    if (choice.kind == UpgradeKind::Weapon && !weapons_.HasWeapon(choice.weapon) &&
        weapons_.WeaponCount() >= weapons_.WeaponSlotLimit()) {
        pendingReplacementWeapon_ = choice.weapon;
        pendingWeaponReplacement_ = true;
        replacementFromLevelUp_ = true;
        return;
    }
    const bool applied = choice.kind == UpgradeKind::Weapon
                             ? weapons_.AddOrUpgradeWeapon(choice.weapon)
                             : weapons_.AddOrUpgradePassive(choice.passive, player_.stats);
    if (applied) {
        player_.SanitizeStats();
        --pendingLevelUps_;
    }
}

void GameSession::PrepareChestRewards() {
    chestChoices_ = {};
    chestChoiceCount_ = 0;
    for (int raw = 0; raw < static_cast<int>(WeaponType::Count) && chestChoiceCount_ < 3; ++raw) {
        const WeaponType type = static_cast<WeaponType>(raw);
        if (!weapons_.IsEvolutionEligible(type)) continue;
        const WeaponEvolutionDefinition& evolution = GetEvolutionDefinition(type);
        chestChoices_[static_cast<std::size_t>(chestChoiceCount_++)] = {
            ChestRewardKind::Evolution, type, evolution.requiredPassive,
            evolution.name, evolution.behavior};
    }
    if (chestChoiceCount_ > 0) return;

    for (int raw = 0; raw < static_cast<int>(WeaponType::Count) && chestChoiceCount_ < 3; ++raw) {
        const WeaponType type = static_cast<WeaponType>(raw);
        const int level = weapons_.WeaponLevel(type);
        if (level <= 0 || level >= WeaponManager::MaxWeaponLevel || weapons_.IsEvolved(type)) continue;
        const WeaponDefinition& definition = GetWeaponDefinition(type);
        chestChoices_[static_cast<std::size_t>(chestChoiceCount_++)] = {
            ChestRewardKind::WeaponUpgrade, type, PassiveType::SwiftBoots,
            definition.name, GetWeaponLevelEffect(type, level + 1)};
    }
    for (int raw = 0; raw < static_cast<int>(PassiveType::Count) && chestChoiceCount_ < 3; ++raw) {
        const PassiveType type = static_cast<PassiveType>(raw);
        const int level = weapons_.PassiveLevel(type);
        if (level <= 0 || level >= WeaponManager::MaxPassiveLevel) continue;
        const PassiveDefinition& definition = GetPassiveDefinition(type);
        chestChoices_[static_cast<std::size_t>(chestChoiceCount_++)] = {
            ChestRewardKind::PassiveUpgrade, WeaponType::ArcBolt, type,
            definition.name, GetPassiveLevelEffect(type, level + 1)};
    }
    if (chestChoiceCount_ == 0) {
        chestChoices_[0] = {ChestRewardKind::Recovery, WeaponType::ArcBolt,
                            PassiveType::SwiftBoots, "FULL RESTORATION",
                            "Restore all HP and gain 100 XP."};
        chestChoiceCount_ = 1;
    }
}

void GameSession::PrepareExpeditionStageReward(bool eliteQuality) {
    std::vector<ChestRewardChoice> candidates;
    candidates.reserve(static_cast<std::size_t>(WeaponType::Count) +
                       static_cast<std::size_t>(PassiveType::Count) + 2);
    for (int raw = 0; raw < static_cast<int>(WeaponType::Count); ++raw) {
        const WeaponType type = static_cast<WeaponType>(raw);
        if (weapons_.IsEvolutionEligible(type)) {
            const auto& evolution = GetEvolutionDefinition(type);
            candidates.push_back({ChestRewardKind::Evolution, type, evolution.requiredPassive,
                                  evolution.name, evolution.behavior, Rarity::Epic});
            continue;
        }
        const int level = weapons_.WeaponLevel(type);
        if (level >= WeaponManager::MaxWeaponLevel || weapons_.IsEvolved(type)) continue;
        const auto& definition = GetWeaponDefinition(type);
        candidates.push_back({ChestRewardKind::WeaponUpgrade, type, PassiveType::SwiftBoots,
                              definition.name, GetWeaponLevelEffect(type, level + 1),
                              definition.rarity});
    }
    for (int raw = 0; raw < static_cast<int>(PassiveType::Count); ++raw) {
        const PassiveType type = static_cast<PassiveType>(raw);
        const int level = weapons_.PassiveLevel(type);
        if (level >= WeaponManager::MaxPassiveLevel ||
            (level == 0 && weapons_.PassiveCount() >= WeaponManager::MaxPassiveSlots)) continue;
        const auto& definition = GetPassiveDefinition(type);
        candidates.push_back({ChestRewardKind::PassiveUpgrade, WeaponType::ArcBolt, type,
                              definition.name, GetPassiveLevelEffect(type, level + 1),
                              definition.rarity});
    }
    candidates.push_back({ChestRewardKind::StageRecovery, WeaponType::ArcBolt,
                          PassiveType::SwiftBoots, "FIELD MENDING",
                          "Recover 30% maximum HP.", eliteQuality ? Rarity::Rare : Rarity::Common});
    candidates.push_back({ChestRewardKind::Experience, WeaponType::ArcBolt,
                          PassiveType::SwiftBoots, "ANCIENT INSIGHT",
                          "Gain 150 XP.", eliteQuality ? Rarity::Epic : Rarity::Rare});

    chestChoices_ = {};
    chestChoiceCount_ = 0;
    // The first Expedition reward strongly establishes the dual-weapon loadout.
    if (weapons_.WeaponCount() == 1) {
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            if (candidates[i].kind == ChestRewardKind::WeaponUpgrade &&
                !weapons_.HasWeapon(candidates[i].weapon)) {
                chestChoices_[static_cast<std::size_t>(chestChoiceCount_++)] = candidates[i];
                candidates.erase(candidates.begin() + static_cast<std::ptrdiff_t>(i));
                break;
            }
        }
    }
    while (chestChoiceCount_ < 3 && !candidates.empty()) {
        std::vector<std::size_t> preferred;
        if (eliteQuality) for (std::size_t i = 0; i < candidates.size(); ++i)
            if (candidates[i].rarity != Rarity::Common) preferred.push_back(i);
        const std::size_t selected = preferred.empty()
            ? static_cast<std::size_t>(GetRandomValue(0, static_cast<int>(candidates.size()) - 1))
            : preferred[static_cast<std::size_t>(GetRandomValue(0, static_cast<int>(preferred.size()) - 1))];
        chestChoices_[static_cast<std::size_t>(chestChoiceCount_++)] = candidates[selected];
        candidates.erase(candidates.begin() + static_cast<std::ptrdiff_t>(selected));
    }
    pendingChestReward_ = chestChoiceCount_ > 0;
}

void GameSession::SelectChestReward(int index) {
    if (!pendingChestReward_ || index < 0 || index >= chestChoiceCount_) return;
    const ChestRewardChoice& choice = chestChoices_[static_cast<std::size_t>(index)];
    bool applied = false;
    if (choice.kind == ChestRewardKind::Evolution) applied = weapons_.EvolveWeapon(choice.weapon);
    else if (choice.kind == ChestRewardKind::WeaponUpgrade) {
        if (!weapons_.HasWeapon(choice.weapon) &&
            weapons_.WeaponCount() >= weapons_.WeaponSlotLimit()) {
            pendingReplacementWeapon_ = choice.weapon;
            pendingWeaponReplacement_ = true;
            replacementFromLevelUp_ = false;
            return;
        }
        applied = weapons_.AddOrUpgradeWeapon(choice.weapon);
    }
    else if (choice.kind == ChestRewardKind::PassiveUpgrade)
        applied = weapons_.AddOrUpgradePassive(choice.passive, player_.stats);
    else if (choice.kind == ChestRewardKind::Recovery) {
        player_.HealToFull();
        AddXP(100);
        applied = true;
    } else if (choice.kind == ChestRewardKind::StageRecovery) {
        player_.Heal(player_.stats.maxHP * 0.30f);
        applied = true;
    } else if (choice.kind == ChestRewardKind::Experience) {
        AddXP(150);
        applied = true;
    }
    if (applied) {
        player_.SanitizeStats();
        if (choice.kind == ChestRewardKind::Evolution) {
            particles_.SpawnBurst(player_.Position(), GOLD, 120, 240.0f,
                                  ParticleKind::Square, ParticlePriority::Important);
            screenShake_.Add(8.0f, 0.45f);
            if (audio_) audio_->Play(AudioCue::Evolution);
        }
        pendingChestReward_ = false;
        chestChoiceCount_ = 0;
        statistics_.evolutionsObtained = weapons_.EvolutionCount();
    }
}

bool GameSession::ConfirmWeaponReplacement(int slot) {
    if (!pendingWeaponReplacement_) return false;
    if (!weapons_.ReplaceWeapon(slot, pendingReplacementWeapon_)) return false;
    pendingWeaponReplacement_ = false;
    if (replacementFromLevelUp_) {
        if (pendingLevelUps_ > 0) --pendingLevelUps_;
        choiceCount_ = 0;
    } else {
        pendingChestReward_ = false;
        chestChoiceCount_ = 0;
    }
    replacementFromLevelUp_ = false;
    return true;
}
