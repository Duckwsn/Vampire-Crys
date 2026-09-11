#pragma once

#include "game_modes/ExpeditionMode.h"
#include "game_modes/SurvivalMode.h"

class GameSession;

class GameModeManager {
public:
    bool SelectMode(GameModeType type);
    bool ActivateMode(GameModeType type, GameSession& session,
                      CharacterId character = CharacterId::Hunter,
                      SurvivalRunConfig config = {}, ExpeditionRunConfig expeditionConfig = {});
    RunOutcome UpdateActiveMode(float deltaTime, GameSession& session);
    bool RestartActiveMode(GameSession& session);
    void ExitActiveMode(GameSession& session);

    bool HasActiveMode() const { return activeMode_ != nullptr; }
    GameModeType ActiveType() const;
    const IGameMode* GetActiveMode() const { return activeMode_; }
    IGameMode* GetActiveMode() { return activeMode_; }
    const RunContext& Context() const { return context_; }
    const RunResult& Result() const { return result_; }
    ModeHUDData HUDData() const;

    void DebugSkipTime(float seconds, GameSession& session);
    void DebugSpawnEnemies(int count, GameSession& session);
    void DebugForceEvent(GameSession& session);
    void DebugEndEvent();
    void DebugForceSpecialWave();
    void DebugSpawnMiniboss(GameSession& session, bool stalker);
    void DebugCompleteStage(GameSession& session);
    void DebugOpenReward(GameSession& session);
    void DebugGoToBoss(GameSession& session);
    bool SelectRoute(int option, GameSession& session);
    const ExpeditionDirector& ExpeditionRoute() const { return expedition_.Director(); }

private:
    IGameMode* ModeFor(GameModeType type);
    void CompleteRun(RunOutcome outcome, const GameSession& session);

    SurvivalMode survival_;
    ExpeditionMode expedition_;
    IGameMode* selectedMode_ = nullptr;
    IGameMode* activeMode_ = nullptr;
    RunContext context_{};
    RunResult result_{};
    bool runEndNotified_ = false;
};
