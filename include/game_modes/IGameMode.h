#pragma once

#include "game_modes/GameModeTypes.h"

class GameSession;

class IGameMode {
public:
    virtual ~IGameMode() = default;

    virtual GameModeType Type() const = 0;
    virtual bool IsAvailable() const = 0;
    virtual void OnEnter() = 0;
    virtual void Reset() = 0;
    virtual void OnRunStart(GameSession& session) = 0;
    virtual void Update(float deltaTime, GameSession& session) = 0;
    virtual void AfterSharedUpdate(GameSession& session) = 0;
    virtual void OnRunEnd(RunOutcome outcome) = 0;
    virtual void OnExit() = 0;
    virtual bool HasWon() const = 0;
    virtual ModeHUDData HUDData() const = 0;

    virtual void DebugSkipTime(float, GameSession&) {}
    virtual void DebugSpawnEnemies(int, GameSession&) {}
    virtual void DebugForceEvent(GameSession&) {}
    virtual void DebugEndEvent() {}
    virtual void DebugForceSpecialWave() {}
    virtual void DebugSpawnMiniboss(GameSession&, bool) {}
    virtual void DebugCompleteStage(GameSession&) {}
    virtual void DebugOpenReward(GameSession&) {}
    virtual void DebugGoToBoss(GameSession&) {}
    virtual bool SelectRoute(int, GameSession&) { return false; }
};
