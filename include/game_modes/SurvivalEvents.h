#pragma once

#include <array>
#include <cstdint>

#include "raylib.h"

class EnemyManager;

enum class SurvivalEventType { None, BloodMoon, TheSwarm, EliteHunt, Count };
enum class SurvivalEventState { Inactive, Telegraph, Active, Ending, Cooldown };

struct SurvivalEventModifiers {
    float spawnIntervalMultiplier = 1.0f;
    float enemySpeedMultiplier = 1.0f;
    float eliteChanceMultiplier = 1.0f;
    float normalPressureMultiplier = 1.0f;
    bool forceSwarmComposition = false;
};

class SurvivalEventManager {
public:
    void Reset(std::uint32_t seed);
    void Update(float deltaTime, float elapsedTime, bool encountersBlocked,
                Vector2 playerPosition, EnemyManager& enemies,
                float intervalMultiplier = 1.0f, int maximumEvents = 3,
                SurvivalEventType preferred = SurvivalEventType::None);
    void ForceNext();
    void EndActive();

    SurvivalEventType Type() const { return type_; }
    SurvivalEventState State() const { return state_; }
    SurvivalEventModifiers Modifiers() const;
    bool IsActive() const { return state_ == SurvivalEventState::Telegraph ||
                                  state_ == SurvivalEventState::Active ||
                                  state_ == SurvivalEventState::Ending; }
    float Remaining() const { return timer_; }
    int CompletedCount() const { return completedCount_; }
    const char* Name() const;
    const char* Subtitle() const;
    std::uint32_t Seed() const { return seed_; }

private:
    std::uint32_t Random();
    void BeginEvent(SurvivalEventType preferred);
    float ActiveDuration() const;
    void SpawnEliteTarget(Vector2 playerPosition, EnemyManager& enemies);

    std::array<bool, static_cast<std::size_t>(SurvivalEventType::Count)> used_{};
    SurvivalEventType type_ = SurvivalEventType::None;
    SurvivalEventType previous_ = SurvivalEventType::None;
    SurvivalEventState state_ = SurvivalEventState::Inactive;
    float timer_ = 0.0f;
    float nextTrigger_ = 125.0f;
    float eliteSpawnTimer_ = 0.0f;
    int elitesSpawned_ = 0;
    int completedCount_ = 0;
    std::uint32_t seed_ = 1;
    std::uint32_t randomState_ = 1;
    bool forceRequested_ = false;
};
