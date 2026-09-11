#pragma once

#include <array>
#include <string>
#include <vector>

#include "gameplay/Definitions.h"
#include "raylib.h"

class EnemyManager;

enum class SpecialWaveType { None, SwarmWave, BruteWall, CultistCircle, BomberRush, MixedAssault, Count };
struct WaveRuntimeModifiers {
    float spawnIntervalMultiplier = 1.0f;
    float eliteChanceMultiplier = 1.0f;
    float pressureMultiplier = 1.0f;
    bool forceSwarmComposition = false;
    float enemyHpMultiplier = 1.0f;
    float enemyDamageMultiplier = 1.0f;
    float enemySpeedMultiplier = 1.0f;
    float specialWaveIntervalMultiplier = 1.0f;
    bool forceDurableComposition = false;
};

struct WaveSegment {
    std::string name;
    float start = 0.0f;
    float end = 60.0f;
    float spawnInterval = 1.0f;
    int spawnCount = 1;
    int enemyCap = 100;
    int maxCultists = 0;
    int maxBombers = 0;
    std::array<float, static_cast<std::size_t>(EnemyType::Count)> weights{};
};

struct DifficultyState {
    float hp = 1.0f;
    float damage = 1.0f;
    float speed = 1.0f;
    float xp = 1.0f;
    float eliteChance = 0.0f;
};

class WaveDirector {
public:
    static constexpr float DefaultRunDuration = 900.0f;

    WaveDirector();
    void Reset();
    void Update(float deltaTime, float elapsedTime, Vector2 playerPosition,
                float cameraZoom, EnemyManager& enemies, bool bossEncounterActive = false,
                WaveRuntimeModifiers modifiers = {}, bool encountersBlocked = false);
    void SpawnStress(int count, float elapsedTime, Vector2 playerPosition,
                     EnemyManager& enemies);

    float RunDuration() const { return runDuration_; }
    const WaveSegment& CurrentWave() const;
    int CurrentWaveIndex() const { return currentWaveIndex_; }
    DifficultyState CurrentDifficulty() const { return difficulty_; }
    bool LoadedExternalConfig() const { return loadedExternalConfig_; }
    SpecialWaveType ActiveSpecialWave() const { return specialWave_; }
    const char* ActiveSpecialWaveName() const;
    float SpecialWaveRemaining() const { return specialWaveTimer_; }
    float ThreatBudget() const { return threatBudget_; }
    void ForceSpecialWave(SpecialWaveType type);

private:
    void LoadConfiguration();
    void LoadSafeDefaults();
    void SelectWave(float elapsedTime);
    DifficultyState CalculateDifficulty(float elapsedTime) const;
    EnemyType SelectEnemyType(const WaveSegment& wave, const EnemyManager& enemies) const;
    Vector2 SpawnPosition(Vector2 playerPosition, float cameraZoom) const;
    bool RollElite(float chance) const;
    void StartSpecialWave(float elapsedTime);
    void SpawnSpecialGroup(Vector2 playerPosition, float cameraZoom, EnemyManager& enemies);
    float ThreatCost(EnemyType type) const;

    std::vector<WaveSegment> waves_;
    float runDuration_ = DefaultRunDuration;
    float spawnTimer_ = 0.0f;
    int currentWaveIndex_ = 0;
    DifficultyState difficulty_{};
    bool loadedExternalConfig_ = false;
    SpecialWaveType specialWave_ = SpecialWaveType::None;
    SpecialWaveType forcedSpecialWave_ = SpecialWaveType::None;
    float specialWaveTimer_ = 0.0f;
    float nextSpecialWave_ = 78.0f;
    float threatBudget_ = 0.0f;
    int specialSequence_ = 0;
    int specialBurst_ = 0;
    WaveRuntimeModifiers runtimeModifiers_{};
};
