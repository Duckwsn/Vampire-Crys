#pragma once

#include "game_modes/ExpeditionDirector.h"
#include "game_modes/IGameMode.h"

class ExpeditionMode final : public IGameMode {
public:
    GameModeType Type() const override { return GameModeType::Expedition; }
    bool IsAvailable() const override { return true; }
    void OnEnter() override {}
    void Reset() override;
    void OnRunStart(GameSession&) override;
    void Update(float, GameSession&) override;
    void AfterSharedUpdate(GameSession&) override;
    void OnRunEnd(RunOutcome) override {}
    void OnExit() override {}
    bool HasWon() const override { return director_.HasWon(); }
    ModeHUDData HUDData() const override;
    void SetRunConfig(ExpeditionRunConfig config) { config_ = config; }
    void DebugCompleteStage(GameSession& session) override { director_.DebugCompleteStage(session); }
    void DebugOpenReward(GameSession& session) override { director_.DebugOpenReward(session); }
    void DebugGoToBoss(GameSession& session) override { director_.DebugGoToBoss(session); }
    bool SelectRoute(int option, GameSession& session) override { return director_.SelectRoute(option, session); }
    const ExpeditionDirector& Director() const { return director_; }

private:
    ExpeditionDirector director_;
    ExpeditionRunConfig config_{};
    int enemiesRemaining_ = 0;
};
