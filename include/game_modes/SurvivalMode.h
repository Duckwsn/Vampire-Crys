#pragma once

#include <array>

#include "game_modes/IGameMode.h"
#include "game_modes/SurvivalEvents.h"
#include "game_modes/SurvivalEndgame.h"
#include "systems/WaveDirector.h"

class SurvivalMode final : public IGameMode {
public:
    GameModeType Type() const override { return GameModeType::Survival; }
    bool IsAvailable() const override { return true; }
    void OnEnter() override {}
    void Reset() override;
    void OnRunStart(GameSession& session) override;
    void Update(float deltaTime, GameSession& session) override;
    void AfterSharedUpdate(GameSession& session) override;
    void OnRunEnd(RunOutcome) override {}
    void OnExit() override {}
    bool HasWon() const override { return finalBossDefeated_; }
    ModeHUDData HUDData() const override;
    void DebugSkipTime(float seconds, GameSession& session) override;
    void DebugSpawnEnemies(int count, GameSession& session) override;
    void DebugForceEvent(GameSession&) override { events_.ForceNext(); }
    void DebugEndEvent() override { events_.EndActive(); }
    void DebugForceSpecialWave() override;
    void DebugSpawnMiniboss(GameSession& session, bool stalker) override;

    const WaveDirector& Director() const { return director_; }
    const SurvivalEventManager& Events() const { return events_; }
    int PendingEncounterCount() const;
    void SetRunConfig(SurvivalRunConfig config) { config_ = ResolveSurvivalConfig(config); }
    const SurvivalRunConfig& RunConfig() const { return config_; }
    int EndlessCycle() const { return endlessCycle_; }

private:
    void ScheduleDueEncounters(float elapsedTime);
    void TryStartNextEncounter(GameSession& session);
    void TryStartMiniboss(GameSession& session);

    WaveDirector director_;
    SurvivalEventManager events_;
    std::array<bool, 3> encounterDue_{};
    std::array<bool, 3> encounterCompleted_{};
    float encounterDelay_ = 0.0f;
    bool finalBossDefeated_ = false;
    std::array<bool, 3> minibossDue_{};
    std::array<bool, 3> minibossCompleted_{};
    int specialDebugIndex_ = 0;
    unsigned int runSeed_ = 1;
    SurvivalRunConfig config_{};
    RunModifiers modifiers_{};
    int endlessCycle_ = 1;
    bool endlessBossDue_ = false;
    float nextEndlessBoss_ = 1200.0f;
    float nextBonusMiniboss_ = 135.0f;
    int endlessBossSequence_ = 0;
};
