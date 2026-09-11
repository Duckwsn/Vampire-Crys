#include "game_modes/SurvivalEvents.h"

#include <algorithm>
#include <cmath>

#include "gameplay/Definitions.h"
#include "systems/EnemyManager.h"

void SurvivalEventManager::Reset(std::uint32_t seed) {
    used_ = {};
    type_ = previous_ = SurvivalEventType::None;
    state_ = SurvivalEventState::Inactive;
    timer_ = eliteSpawnTimer_ = 0.0f;
    nextTrigger_ = 125.0f + static_cast<float>(seed % 31u);
    elitesSpawned_ = completedCount_ = 0;
    seed_ = seed == 0 ? 1u : seed;
    randomState_ = seed_;
    forceRequested_ = false;
}

std::uint32_t SurvivalEventManager::Random() {
    randomState_ ^= randomState_ << 13u;
    randomState_ ^= randomState_ >> 17u;
    randomState_ ^= randomState_ << 5u;
    return randomState_;
}

void SurvivalEventManager::Update(float deltaTime, float elapsedTime, bool blocked,
                                  Vector2 playerPosition, EnemyManager& enemies,
                                  float intervalMultiplier, int maximumEvents,
                                  SurvivalEventType preferred) {
    const float bossMilestone = elapsedTime < 300.0f ? 300.0f :
                                (elapsedTime < 600.0f ? 600.0f : 900.0f);
    const float nextBoss = elapsedTime < 900.0f ? bossMilestone :
                           (std::floor(elapsedTime / 300.0f) + 1.0f) * 300.0f;
    if (state_ == SurvivalEventState::Inactive && (forceRequested_ || elapsedTime >= nextTrigger_) &&
        !blocked && nextBoss - elapsedTime > 46.0f && completedCount_ < maximumEvents)
        BeginEvent(preferred);
    if (state_ == SurvivalEventState::Inactive) return;
    timer_ -= deltaTime;
    if (state_ == SurvivalEventState::Telegraph && timer_ <= 0.0f) {
        state_ = SurvivalEventState::Active;
        timer_ = ActiveDuration();
        eliteSpawnTimer_ = 0.2f;
    } else if (state_ == SurvivalEventState::Active) {
        if (type_ == SurvivalEventType::EliteHunt && elitesSpawned_ < 3) {
            eliteSpawnTimer_ -= deltaTime;
            if (eliteSpawnTimer_ <= 0.0f) {
                SpawnEliteTarget(playerPosition, enemies);
                eliteSpawnTimer_ = 7.5f;
            }
        }
        if (timer_ <= 0.0f) {
            if (type_ == SurvivalEventType::EliteHunt) enemies.DespawnBySource(SpawnSource::EliteHunt);
            state_ = SurvivalEventState::Ending; timer_ = 1.5f;
        }
    } else if (state_ == SurvivalEventState::Ending && timer_ <= 0.0f) {
        previous_ = type_; type_ = SurvivalEventType::None;
        state_ = SurvivalEventState::Cooldown; timer_ = 38.0f;
        ++completedCount_;
        nextTrigger_ = elapsedTime + (205.0f + static_cast<float>(Random() % 41u)) *
                                      std::clamp(intervalMultiplier, 0.35f, 1.5f);
    } else if (state_ == SurvivalEventState::Cooldown && timer_ <= 0.0f) {
        state_ = SurvivalEventState::Inactive;
    }
}

void SurvivalEventManager::BeginEvent(SurvivalEventType preferred) {
    std::array<SurvivalEventType, 3> candidates{{SurvivalEventType::BloodMoon,
                                                SurvivalEventType::TheSwarm,
                                                SurvivalEventType::EliteHunt}};
    if (preferred != SurvivalEventType::None) {
        type_ = preferred;
        state_ = SurvivalEventState::Telegraph;
        timer_ = 3.0f; elitesSpawned_ = 0; forceRequested_ = false;
        return;
    }
    bool allUsed = true;
    for (SurvivalEventType candidate : candidates)
        allUsed &= used_[static_cast<std::size_t>(candidate)];
    if (allUsed) used_ = {};
    const std::size_t start = Random() % candidates.size();
    type_ = SurvivalEventType::None;
    for (std::size_t offset = 0; offset < candidates.size(); ++offset) {
        const SurvivalEventType candidate = candidates[(start + offset) % candidates.size()];
        if (!used_[static_cast<std::size_t>(candidate)] && candidate != previous_) {
            type_ = candidate; break;
        }
    }
    if (type_ == SurvivalEventType::None)
        for (SurvivalEventType candidate : candidates)
            if (!used_[static_cast<std::size_t>(candidate)]) { type_ = candidate; break; }
    if (type_ == SurvivalEventType::None) return;
    used_[static_cast<std::size_t>(type_)] = true;
    state_ = SurvivalEventState::Telegraph;
    timer_ = 3.0f;
    elitesSpawned_ = 0;
    forceRequested_ = false;
}

void SurvivalEventManager::SpawnEliteTarget(Vector2 playerPosition, EnemyManager& enemies) {
    const float angle = static_cast<float>(Random() % 3600u) * 0.1f * DEG2RAD;
    const Vector2 position{
        std::clamp(playerPosition.x + std::cos(angle) * 760.0f, -1960.0f, 1960.0f),
        std::clamp(playerPosition.y + std::sin(angle) * 760.0f, -1960.0f, 1960.0f)};
    const EnemyType type = elitesSpawned_ % 2 == 0 ? EnemyType::ShieldedAcolyte : EnemyType::Brute;
    if (enemies.Spawn(type, position, 1.25f, 1.1f, 1.0f, 1.8f, true,
                      SpawnSource::EliteHunt)) ++elitesSpawned_;
}

SurvivalEventModifiers SurvivalEventManager::Modifiers() const {
    if (state_ != SurvivalEventState::Active) return {};
    if (type_ == SurvivalEventType::BloodMoon) return {0.72f, 1.08f, 1.5f, 1.0f, false};
    if (type_ == SurvivalEventType::TheSwarm) return {0.68f, 1.04f, 0.7f, 1.0f, true};
    if (type_ == SurvivalEventType::EliteHunt) return {1.0f, 1.0f, 1.0f, 0.72f, false};
    return {};
}

float SurvivalEventManager::ActiveDuration() const {
    if (type_ == SurvivalEventType::BloodMoon) return 34.0f;
    if (type_ == SurvivalEventType::TheSwarm) return 27.0f;
    return 29.0f;
}

void SurvivalEventManager::ForceNext() { forceRequested_ = true; }
void SurvivalEventManager::EndActive() { if (IsActive()) { state_ = SurvivalEventState::Ending; timer_ = 0.25f; } }

const char* SurvivalEventManager::Name() const {
    switch (type_) {
        case SurvivalEventType::BloodMoon: return "BLOOD MOON";
        case SurvivalEventType::TheSwarm: return "THE SWARM";
        case SurvivalEventType::EliteHunt: return "ELITE HUNT";
        default: return "NONE";
    }
}

const char* SurvivalEventManager::Subtitle() const {
    switch (type_) {
        case SurvivalEventType::BloodMoon: return "The horde quickens";
        case SurvivalEventType::TheSwarm: return "Fast bodies flood the arena";
        case SurvivalEventType::EliteHunt: return "Marked elites carry useful spoils";
        default: return "";
    }
}
