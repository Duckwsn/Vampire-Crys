#pragma once

#include <array>
#include <cstdint>

enum class DifficultyTier { Normal, Hard, Nightmare, Count };
enum class MutatorId {
    GlassCannon, HyperHorde, Titanic, EliteInfestation,
    BloodRush, ScarceRecovery, Eventful, ChaoticWaves, Count
};
enum class ChallengeId {
    None, TheSwarm, Titans, NightOfElites, BloodMoon, BossHunter, FragilePower, Count
};

struct MutatorDefinition {
    MutatorId id;
    const char* name;
    const char* description;
    int scoreImpact;
};

struct ChallengeDefinition {
    ChallengeId id;
    const char* name;
    const char* description;
    DifficultyTier difficulty;
    int ascension;
    std::uint16_t mutatorMask;
};

struct SurvivalRunConfig {
    DifficultyTier difficulty = DifficultyTier::Normal;
    int ascension = 0;
    bool endless = false;
    ChallengeId challenge = ChallengeId::None;
    std::uint16_t mutatorMask = 0;

    void Sanitize();
    int MutatorCount() const;
    bool HasMutator(MutatorId id) const;
    bool ToggleMutator(MutatorId id);
};

struct RunModifiers {
    float enemyHp = 1.0f;
    float enemyDamage = 1.0f;
    float enemySpeed = 1.0f;
    float spawnInterval = 1.0f;
    float spawnPressure = 1.0f;
    float eliteChance = 1.0f;
    float minibossHp = 1.0f;
    float bossHp = 1.0f;
    float bossDamage = 1.0f;
    float playerDamage = 1.0f;
    float playerHp = 1.0f;
    float playerSpeed = 1.0f;
    float healthDropChance = 1.0f;
    float eventInterval = 1.0f;
    float specialWaveInterval = 1.0f;
    float scoreMultiplier = 1.0f;
    bool forceSwarm = false;
    bool forceDurable = false;
    bool preferBloodMoon = false;
    bool bossHunter = false;
};

struct ScoreBreakdown {
    long long baseScore = 0;
    float difficultyMultiplier = 1.0f;
    float ascensionMultiplier = 1.0f;
    float mutatorMultiplier = 1.0f;
    long long finalScore = 0;
};

const MutatorDefinition& GetMutatorDefinition(MutatorId id);
const ChallengeDefinition& GetChallengeDefinition(ChallengeId id);
const char* DifficultyName(DifficultyTier tier);
const char* ChallengeName(ChallengeId id);
SurvivalRunConfig ResolveSurvivalConfig(SurvivalRunConfig config);
RunModifiers BuildRunModifiers(const SurvivalRunConfig& config, int endlessCycle = 1);
ScoreBreakdown CalculateSurvivalScore(int eligibleKills, int elites, int minibosses,
                                      int bosses, float elapsedTime,
                                      const SurvivalRunConfig& config);
