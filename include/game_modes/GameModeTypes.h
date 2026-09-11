#pragma once

#include "gameplay/Characters.h"
#include "game_modes/SurvivalEndgame.h"
#include <cstdint>

enum class GameModeType {
    Survival,
    Expedition
};

constexpr const char* GameModeName(GameModeType type) {
    return type == GameModeType::Survival ? "SURVIVAL" : "EXPEDITION";
}

enum class RunOutcome {
    None,
    Victory,
    Defeat
};

struct ExpeditionRunConfig { std::uint32_t seed = 0; };

struct RunContext {
    GameModeType mode = GameModeType::Survival;
    float elapsedTime = 0.0f;
    RunOutcome outcome = RunOutcome::None;
    CharacterId character = CharacterId::Hunter;
    SurvivalRunConfig survivalConfig{};
    ExpeditionRunConfig expeditionConfig{};
};

struct RunResult {
    GameModeType mode = GameModeType::Survival;
    RunOutcome outcome = RunOutcome::None;
    bool victory = false;
    float timePlayed = 0.0f;
    int levelReached = 1;
    int kills = 0;
    int elitesKilled = 0;
    int bossesKilled = 0;
    double damageDealt = 0.0;
    double damageTaken = 0.0;
    int chestsOpened = 0;
    int evolutionsObtained = 0;
    CharacterId character = CharacterId::Hunter;
    SurvivalRunConfig survivalConfig{};
    ScoreBreakdown score{};
    int endlessCycles = 0;
    int expeditionStagesCompleted = 0;
    unsigned int expeditionSeed = 0;
};

struct ModeHUDData {
    GameModeType mode = GameModeType::Survival;
    const char* progressLabel = "";
    const char* progressName = "";
    float duration = 0.0f;
    int progressIndex = 0;
    int pendingEncounters = 0;
    float spawnInterval = 0.0f;
    int spawnCount = 0;
    int enemyCap = 0;
    const char* eventName = "NONE";
    const char* eventSubtitle = "";
    float eventRemaining = 0.0f;
    bool eventActive = false;
    const char* specialWaveName = "NONE";
    float specialWaveRemaining = 0.0f;
    const char* minibossName = "NONE";
    float threatBudget = 0.0f;
    unsigned int runSeed = 0;
    DifficultyTier difficulty = DifficultyTier::Normal;
    ChallengeId challenge = ChallengeId::None;
    int ascension = 0;
    int endlessCycle = 1;
    int mutatorCount = 0;
    float scoreMultiplier = 1.0f;
    bool endless = false;
    int stageCount = 0;
    int stagesCompleted = 0;
    int enemiesRemaining = 0;
    const char* objective = "";
    float stageIntroRemaining = 0.0f;
    float transitionAlpha = 0.0f;
    bool exitOpen = false;
    bool routeChoiceActive = false;
    int routeOptionCount = 0;
    const char* routeOptionA = "";
    const char* routeOptionB = "";
    const char* routeTypeA = "";
    const char* routeTypeB = "";
};
