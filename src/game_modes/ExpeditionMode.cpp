#include "game_modes/ExpeditionMode.h"

#include "gameplay/GameSession.h"

void ExpeditionMode::Reset() {
    if (config_.seed == 0) config_.seed = static_cast<unsigned int>(GetRandomValue(1, 0x3fffffff));
    director_.Reset(config_.seed);
}

void ExpeditionMode::OnRunStart(GameSession& session) {
    session.SetManualCombat(true);
    director_.Start(session);
}
void ExpeditionMode::Update(float deltaTime, GameSession& session) { director_.Update(deltaTime, session); }
void ExpeditionMode::AfterSharedUpdate(GameSession& session) {
    director_.AfterSharedUpdate(session);
    enemiesRemaining_ = director_.EnemiesRemaining(session);
}

ModeHUDData ExpeditionMode::HUDData() const {
    ModeHUDData data{};
    data.mode = GameModeType::Expedition;
    data.progressLabel = "STAGE";
    data.progressName = director_.CurrentStage().name;
    data.progressIndex = director_.StageIndex() + 1;
    data.stageCount = director_.StageCount();
    data.stagesCompleted = director_.StagesCompleted();
    data.objective = director_.CurrentStage().objective;
    data.enemiesRemaining = enemiesRemaining_;
    data.stageIntroRemaining = director_.IntroRemaining();
    data.transitionAlpha = director_.TransitionAlpha();
    data.exitOpen = director_.Phase() == ExpeditionPhase::ExitOpen;
    data.routeChoiceActive = director_.Phase() == ExpeditionPhase::RouteChoice;
    data.routeOptionCount = director_.RouteOptionCount();
    auto typeName = [](ExpeditionEncounterType type) {
        return type == ExpeditionEncounterType::Elite ? "ELITE" :
               (type == ExpeditionEncounterType::Reward ? "REWARD" :
               (type == ExpeditionEncounterType::Boss ? "BOSS" : "COMBAT"));
    };
    if (data.routeOptionCount > 0) {
        data.routeOptionA = director_.RouteOption(0).name;
        data.routeTypeA = typeName(director_.RouteOptionNode(0).type);
    }
    if (data.routeOptionCount > 1) {
        data.routeOptionB = director_.RouteOption(1).name;
        data.routeTypeB = typeName(director_.RouteOptionNode(1).type);
    }
    data.runSeed = director_.Seed();
    return data;
}
