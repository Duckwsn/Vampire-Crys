#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "game_modes/ExpeditionMap.h"
#include "gameplay/Definitions.h"
#include "systems/BossManager.h"

class GameSession;

enum class ExpeditionEncounterType { Combat, Elite, Reward, Boss };
enum class ExpeditionPhase { Intro, Combat, Reward, ExitOpen, Transition, RouteChoice, Complete };
enum class ExpeditionRouteNodeState { Unavailable, Available, Current, Completed };

struct ExpeditionRouteNode {
    int id = -1;
    int layer = 0;
    int column = 0;
    ExpeditionEncounterType type = ExpeditionEncounterType::Combat;
    ExpeditionRouteNodeState state = ExpeditionRouteNodeState::Unavailable;
    int catalogIndex = 0;
    std::array<int, 3> connections{{-1, -1, -1}};
    int connectionCount = 0;
};

struct ExpeditionSpawnGroup {
    float delay = 0.0f;
    EnemyType type = EnemyType::Ghoul;
    int count = 0;
    bool elite = false;
};

struct ExpeditionStageDefinition {
    const char* id = "";
    const char* name = "";
    const char* objective = "";
    ExpeditionMapDefinition map;
    ExpeditionEncounterType encounter = ExpeditionEncounterType::Combat;
    std::vector<ExpeditionSpawnGroup> groups;
    BossType boss = BossType::VoidHerald;
};

class ExpeditionDirector {
public:
    void Reset(std::uint32_t seed);
    void Start(GameSession& session);
    void Update(float deltaTime, GameSession& session);
    void AfterSharedUpdate(GameSession& session);
    void DebugCompleteStage(GameSession& session);
    void DebugOpenReward(GameSession& session);
    void DebugGoToBoss(GameSession& session);

    bool HasWon() const { return phase_ == ExpeditionPhase::Complete; }
    int StageIndex() const { return depth_; }
    int StagesCompleted() const { return nodesCompleted_; }
    int StageCount() const { return RouteLayerCount; }
    int EnemiesRemaining(const GameSession& session) const;
    const ExpeditionStageDefinition& CurrentStage() const;
    ExpeditionPhase Phase() const { return phase_; }
    float IntroRemaining() const { return introTimer_; }
    float TransitionAlpha() const;
    std::uint32_t Seed() const { return seed_; }
    const ExpeditionMap& Map() const { return map_; }
    ExpeditionMap& Map() { return map_; }
    int RouteOptionCount() const { return routeOptionCount_; }
    const ExpeditionStageDefinition& RouteOption(int index) const;
    const ExpeditionRouteNode& RouteOptionNode(int index) const;
    bool SelectRoute(int option, GameSession& session);
    const std::vector<ExpeditionRouteNode>& RouteNodes() const { return routeNodes_; }
    const std::vector<int>& SelectedPath() const { return selectedPath_; }
    int CurrentNodeId() const { return currentNodeId_; }
    bool ValidateRoute(std::string* error = nullptr) const;
    static const std::vector<ExpeditionStageDefinition>& Catalog();
    static bool ValidateCatalog(std::string* error = nullptr);

private:
    void GenerateRoute();
    void LoadNode(GameSession& session, int nodeId);
    void LoadStage(GameSession& session, int catalogIndex, int depth);
    void PrepareRouteOptions();
    void CompleteCurrentNode();
    void BeginReward(GameSession& session);
    void SpawnGroup(GameSession& session, const ExpeditionSpawnGroup& group);

    ExpeditionMap map_;
    int depth_ = 0;
    int currentCatalogIndex_ = 0;
    int nextGroup_ = 0;
    float stageTimer_ = 0.0f;
    float introTimer_ = 0.0f;
    float transitionTimer_ = 0.0f;
    std::uint32_t seed_ = 1;
    ExpeditionPhase phase_ = ExpeditionPhase::Intro;
    bool rewardWasOpen_ = false;
    bool bossStarted_ = false;
    bool rewardNodeOnly_ = false;
    static constexpr int RouteLayerCount = 7;
    std::vector<ExpeditionRouteNode> routeNodes_;
    std::vector<int> selectedPath_;
    int currentNodeId_ = -1;
    int nodesCompleted_ = 0;
    std::array<int, 3> routeOptions_{{-1, -1, -1}};
    int routeOptionCount_ = 0;
};
