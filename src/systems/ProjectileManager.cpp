#include "systems/ProjectileManager.h"

#include "gameplay/Balance.h"
#include "systems/AudioManager.h"
#include "ui/VisualStyle.h"

ProjectileManager::ProjectileManager() {
    projectiles_.reserve(Balance::ProjectilePoolSize);
    projectiles_.resize(Balance::ProjectilePoolSize);
}

void ProjectileManager::Reset() {
    for (Projectile& projectile : projectiles_) projectile.active = false;
    explosionEventCount_ = 0;
    playerActiveCount_ = 0;
    enemyActiveCount_ = 0;
    bossActiveCount_ = 0;
    poolWarningEmitted_ = false;
}

bool ProjectileManager::Spawn(const Projectile& specification) {
    for (Projectile& projectile : projectiles_) {
        if (projectile.active) continue;
        projectile = specification;
        projectile.active = true;
        if (projectile.owner == ProjectileOwner::Player) ++playerActiveCount_;
        else if (projectile.owner == ProjectileOwner::Enemy) ++enemyActiveCount_;
        else ++bossActiveCount_;
        if (audio_) {
            if (projectile.owner == ProjectileOwner::Boss) audio_->Play(AudioCue::BossAttack);
            else if (projectile.owner == ProjectileOwner::Player) {
                AudioCue cue = AudioCue::ArcBolt;
                switch (projectile.sourceWeapon) {
                    case WeaponType::ArcBolt: cue = AudioCue::ArcBolt; break;
                    case WeaponType::FlameRing: cue = AudioCue::Flame; break;
                    case WeaponType::GuardianOrbs: cue = AudioCue::Orbital; break;
                    case WeaponType::SpectralFan:
                    case WeaponType::SoulScythe:
                    case WeaponType::BloodNeedles: cue = AudioCue::Spectral; break;
                    case WeaponType::VoidLance:
                    case WeaponType::GravityWell: cue = AudioCue::VoidLance; break;
                    case WeaponType::ThunderCannon: cue = AudioCue::Thunder; break;
                    case WeaponType::FrostShards: cue = AudioCue::ArcBolt; break;
                    case WeaponType::Count: break;
                }
                audio_->Play(cue);
            }
        }
        return true;
    }
    if (!poolWarningEmitted_) {
        TraceLog(LOG_WARNING, "Projectile pool full; shots skipped");
        poolWarningEmitted_ = true;
    }
    return false;
}

void ProjectileManager::Update(float deltaTime) {
    for (Projectile& projectile : projectiles_) {
        if (!projectile.active) continue;
        projectile.position.x += projectile.velocity.x * deltaTime;
        projectile.position.y += projectile.velocity.y * deltaTime;
        projectile.lifetime -= deltaTime;
        projectile.repeatHitCooldown -= deltaTime;
        if (projectile.lifetime <= 0.0f) Deactivate(projectile, projectile.explosive);
    }
}

void ProjectileManager::Draw() const {
    const float time = static_cast<float>(GetTime());
    for (const Projectile& projectile : projectiles_) {
        if (!projectile.active) continue;
        VisualStyle::DrawProjectile(projectile, time);
    }
}

int ProjectileManager::ActiveCount(ProjectileOwner owner) const {
    if (owner == ProjectileOwner::Player) return playerActiveCount_;
    if (owner == ProjectileOwner::Enemy) return enemyActiveCount_;
    return bossActiveCount_;
}

void ProjectileManager::ClearOwner(ProjectileOwner owner) {
    for (Projectile& projectile : projectiles_)
        if (projectile.active && projectile.owner == owner) Deactivate(projectile);
}

void ProjectileManager::Deactivate(Projectile& projectile, bool triggerExplosion) {
    if (!projectile.active) return;
    if (triggerExplosion && projectile.explosive) QueueExplosion(projectile);
    if (projectile.owner == ProjectileOwner::Player) --playerActiveCount_;
    else if (projectile.owner == ProjectileOwner::Enemy) --enemyActiveCount_;
    else --bossActiveCount_;
    projectile.active = false;
}

void ProjectileManager::QueueExplosion(const Projectile& projectile) {
    if (explosionEventCount_ >= static_cast<int>(explosionEvents_.size())) return;
    explosionEvents_[static_cast<std::size_t>(explosionEventCount_++)] = {
        projectile.position, projectile.explosionRadius, projectile.damage,
        projectile.knockback, DamageSource::PlayerArea, projectile.color, projectile.critical};
}
