#include "systems/ParticleManager.h"
#include <algorithm>
#include <cmath>

ParticleManager::ParticleManager() {
    particles_.reserve(1600);
    particles_.resize(1600);
}

void ParticleManager::Reset() {
    for (Particle& particle : particles_) particle.active = false;
    activeCount_ = 0;
    poolWarningEmitted_ = false;
}

void ParticleManager::SpawnBurst(Vector2 position, Color color, int count, float speed,
                                 ParticleKind kind, ParticlePriority priority) {
    for (int spawn = 0; spawn < count; ++spawn) {
        Particle* slot = nullptr;
        for (Particle& particle : particles_) if (!particle.active) { slot = &particle; break; }
        if (slot == nullptr && priority == ParticlePriority::Important) {
            for (Particle& particle : particles_) {
                if (particle.active && particle.priority == ParticlePriority::Low) {
                    slot = &particle;
                    break;
                }
            }
        }
        if (slot == nullptr) {
            if (!poolWarningEmitted_) {
                TraceLog(LOG_WARNING, "Particle pool full; low-priority effects skipped");
                poolWarningEmitted_ = true;
            }
            break;
        }
        const float angle = static_cast<float>(GetRandomValue(0, 3599)) * 0.1f * DEG2RAD;
        const float velocity = speed * static_cast<float>(GetRandomValue(55, 110)) * 0.01f;
        const bool reused = slot->active;
        *slot = {};
        slot->position = position;
        slot->velocity = {std::cos(angle) * velocity, std::sin(angle) * velocity};
        slot->lifetime = static_cast<float>(GetRandomValue(25, 70)) * 0.01f;
        slot->size = static_cast<float>(GetRandomValue(2, priority == ParticlePriority::Important ? 8 : 5));
        slot->alpha = 1.0f;
        slot->color = color;
        slot->kind = kind;
        slot->priority = priority;
        slot->active = true;
        if (!reused) ++activeCount_;
    }
}

void ParticleManager::Update(float deltaTime) {
    for (Particle& particle : particles_) {
        if (!particle.active) continue;
        particle.age += deltaTime;
        particle.position.x += particle.velocity.x * deltaTime;
        particle.position.y += particle.velocity.y * deltaTime;
        particle.velocity.x *= 0.96f;
        particle.velocity.y *= 0.96f;
        particle.alpha = 1.0f - particle.age / particle.lifetime;
        if (particle.age >= particle.lifetime) { particle.active = false; --activeCount_; }
    }
}

void ParticleManager::Draw() const {
    for (const Particle& particle : particles_) {
        if (!particle.active) continue;
        Color color = particle.color;
        color.a = static_cast<unsigned char>(255.0f * std::max(0.0f, particle.alpha));
        if (particle.kind == ParticleKind::Square)
            DrawRectanglePro({particle.position.x, particle.position.y, particle.size * 2.0f,
                              particle.size * 2.0f}, {particle.size, particle.size},
                             particle.age * 180.0f, color);
        else if (particle.kind == ParticleKind::Shard)
            DrawPoly(particle.position, 3, particle.size * 1.5f, particle.age * 220.0f, color);
        else if (particle.kind == ParticleKind::Ring)
            DrawRing(particle.position, particle.size, particle.size + 2.0f, 0.0f, 360.0f,
                     12, color);
        else
            DrawCircleV(particle.position, particle.size, color);
    }
}
