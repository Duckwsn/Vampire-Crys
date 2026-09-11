#include "game_modes/GameModeManager.h"

#include "gameplay/GameSession.h"

bool GameModeManager::SelectMode(GameModeType type) {
    IGameMode* mode = ModeFor(type);
    if (mode == nullptr || !mode->IsAvailable()) return false;
    selectedMode_ = mode;
    return true;
}

bool GameModeManager::ActivateMode(GameModeType type, GameSession& session, CharacterId character,
                                   SurvivalRunConfig config, ExpeditionRunConfig expeditionConfig) {
    if (!SelectMode(type)) return false;
    if (activeMode_ != nullptr) activeMode_->OnExit();

    config = ResolveSurvivalConfig(config);
    if (type == GameModeType::Survival) survival_.SetRunConfig(config);
    else {
        if (expeditionConfig.seed == 0)
            expeditionConfig.seed = static_cast<unsigned int>(GetRandomValue(1, 0x3fffffff));
        expedition_.SetRunConfig(expeditionConfig);
    }
    session.SetSelectedCharacter(character);
    session.SetSurvivalRunConfig(config);
    session.ResetSharedRunSystems();
    selectedMode_->Reset();
    selectedMode_->OnEnter();
    selectedMode_->OnRunStart(session);
    activeMode_ = selectedMode_;
    context_ = {type, 0.0f, RunOutcome::None, character, config, expeditionConfig};
    result_ = {};
    result_.mode = type;
    runEndNotified_ = false;
    return true;
}

RunOutcome GameModeManager::UpdateActiveMode(float deltaTime, GameSession& session) {
    if (activeMode_ == nullptr || context_.outcome != RunOutcome::None)
        return context_.outcome;

    activeMode_->Update(deltaTime, session);
    session.Update(deltaTime);
    activeMode_->AfterSharedUpdate(session);
    context_.elapsedTime = session.ElapsedTime();

    const RunOutcome outcome = session.IsGameOver()
                                   ? RunOutcome::Defeat
                                   : (activeMode_->HasWon() ? RunOutcome::Victory
                                                           : RunOutcome::None);
    if (outcome != RunOutcome::None) CompleteRun(outcome, session);
    return context_.outcome;
}

bool GameModeManager::RestartActiveMode(GameSession& session) {
    return activeMode_ != nullptr && ActivateMode(activeMode_->Type(), session, context_.character,
                                                   context_.survivalConfig, context_.expeditionConfig);
}

void GameModeManager::ExitActiveMode(GameSession& session) {
    if (activeMode_ != nullptr) activeMode_->OnExit();
    activeMode_ = nullptr;
    selectedMode_ = nullptr;
    context_ = {};
    runEndNotified_ = false;
    session.ResetSharedRunSystems();
}

GameModeType GameModeManager::ActiveType() const {
    return activeMode_ != nullptr ? activeMode_->Type() : GameModeType::Survival;
}

ModeHUDData GameModeManager::HUDData() const {
    return activeMode_ != nullptr ? activeMode_->HUDData() : ModeHUDData{};
}

void GameModeManager::DebugSkipTime(float seconds, GameSession& session) {
    if (activeMode_ != nullptr) activeMode_->DebugSkipTime(seconds, session);
}

void GameModeManager::DebugSpawnEnemies(int count, GameSession& session) {
    if (activeMode_ != nullptr) activeMode_->DebugSpawnEnemies(count, session);
}

void GameModeManager::DebugForceEvent(GameSession& session) {
    if (activeMode_ != nullptr) activeMode_->DebugForceEvent(session);
}
void GameModeManager::DebugEndEvent() { if (activeMode_ != nullptr) activeMode_->DebugEndEvent(); }
void GameModeManager::DebugForceSpecialWave() { if (activeMode_ != nullptr) activeMode_->DebugForceSpecialWave(); }
void GameModeManager::DebugSpawnMiniboss(GameSession& session, bool stalker) {
    if (activeMode_ != nullptr) activeMode_->DebugSpawnMiniboss(session, stalker);
}
void GameModeManager::DebugCompleteStage(GameSession& session) { if (activeMode_) activeMode_->DebugCompleteStage(session); }
void GameModeManager::DebugOpenReward(GameSession& session) { if (activeMode_) activeMode_->DebugOpenReward(session); }
void GameModeManager::DebugGoToBoss(GameSession& session) { if (activeMode_) activeMode_->DebugGoToBoss(session); }
bool GameModeManager::SelectRoute(int option, GameSession& session) {
    return activeMode_ != nullptr && activeMode_->SelectRoute(option, session);
}

IGameMode* GameModeManager::ModeFor(GameModeType type) {
    return type == GameModeType::Survival
               ? static_cast<IGameMode*>(&survival_)
               : static_cast<IGameMode*>(&expedition_);
}

void GameModeManager::CompleteRun(RunOutcome outcome, const GameSession& session) {
    context_.outcome = outcome;
    if (!runEndNotified_) {
        activeMode_->OnRunEnd(outcome);
        runEndNotified_ = true;
    }
    const RunStatistics& statistics = session.Statistics();
    result_.mode = activeMode_->Type();
    result_.character = context_.character;
    result_.outcome = outcome;
    result_.victory = outcome == RunOutcome::Victory;
    result_.timePlayed = session.ElapsedTime();
    result_.levelReached = session.GetPlayer().stats.level;
    result_.kills = session.Kills();
    result_.elitesKilled = statistics.elitesKilled;
    result_.bossesKilled = statistics.bossesKilled;
    result_.damageDealt = statistics.damageDealt;
    result_.damageTaken = statistics.damageTaken;
    result_.chestsOpened = statistics.chestsOpened;
    result_.evolutionsObtained = statistics.evolutionsObtained;
    result_.survivalConfig = context_.survivalConfig;
    result_.score = session.CurrentScore();
    result_.endlessCycles = session.EndlessCyclesCompleted();
    if (activeMode_->Type() == GameModeType::Expedition) {
        result_.expeditionStagesCompleted = expedition_.Director().StagesCompleted();
        result_.expeditionSeed = expedition_.Director().Seed();
    }
}
