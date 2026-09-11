#include "game_modes/ExpeditionDirector.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "gameplay/GameSession.h"

namespace {
std::vector<std::string> MapOne() { return {
"........##################........", ".....########################.....",
"...####S###################S###...", "..##############################..",
".######O#########################.", ".###############################X.",
"E################################.", ".####################O############.",
"..##############################..", "...####S###################S###...",
".....########################.....", "........##################........"}; }
std::vector<std::string> MapTwo() { return {
".....############....############.....", "...##############....##############...",
"..####S##########....##########S####..", ".###############......###############.",
"E####################O###############X", ".####################################.",
"..##############....################..", "...####S########....########S#######...",
".....############....############....."}; }
std::vector<std::string> MapThree() { return {
"..........################..........", "......########################......",
"....####S##################S####....", "..##############################....",
"E###############O##################X", "..################################..",
"....####S##################S####....", "......########################......",
"..........################.........."}; }
std::vector<std::string> MapFour() { return {
"....###########......###########....", "..###############..###############..",
".####S########################S####.", ".########O########..########O#######.",
"E##################################X", ".################..#################.",
".####S########################S####.", "..###############..###############..",
"....###########......###########...."}; }
std::vector<std::string> MapFive() { return {
".........####################.........", ".....############################.....",
"...####S######################S####...", "..##################################..",
".################O###################.", "E####################################X",
".########################O###########.", "..##################################..",
"...####S######################S####...", ".....############################.....",
".........####################........."}; }
std::vector<std::string> MapCrimson() { return {
".......##########........##########.......", "....##############....##############....",
"..####S############################S####..", ".##############O#########################.",
"E########################################X", ".#########################O##############.",
"..####S############################S####..", "....##############....##############....",
".......##########........##########......."}; }
std::vector<std::string> MapStalker() { return {
"..........####################..........", "......############################......",
"...####S##########....##########S####...", "..################....################..",
"E##################OO##################X", "..####################################..",
"...####S##########....##########S####...", "......############################......",
"..........####################.........."}; }
std::vector<std::string> MapAshen() { return {
"....##########..........##########....", "..##############......##############..",
".####S##########################S####.", ".############O######O################.",
"E####################################X", ".################O######O############.",
".####S##########################S####.", "..##############......##############..",
"....##########..........##########...."}; }

float DistanceSquared(Vector2 a, Vector2 b) {
    const float x = a.x - b.x, y = a.y - b.y; return x * x + y * y;
}
}

const std::vector<ExpeditionStageDefinition>& ExpeditionDirector::Catalog() {
    static const std::vector<ExpeditionStageDefinition> stages{
        {"shattered_approach", "SHATTERED APPROACH", "Defeat the first host",
         {"shattered_approach", "SHATTERED APPROACH", MapOne()}, ExpeditionEncounterType::Combat,
         {{0.2f, EnemyType::Ghoul, 8, false}, {3.2f, EnemyType::Swarmer, 10, false},
          {6.2f, EnemyType::Cultist, 5, false}}},
        {"divided_halls", "DIVIDED HALLS", "Clear both fractured wings",
         {"divided_halls", "DIVIDED HALLS", MapTwo()}, ExpeditionEncounterType::Combat,
         {{0.2f, EnemyType::Swarmer, 12, false}, {2.8f, EnemyType::ShieldedAcolyte, 5, false},
          {5.8f, EnemyType::Bomber, 6, false}, {8.5f, EnemyType::Ghoul, 10, false}}},
        {"crimson_causeway", "CRIMSON CAUSEWAY", "Break the rushing vanguard",
         {"crimson_causeway", "CRIMSON CAUSEWAY", MapCrimson()}, ExpeditionEncounterType::Combat,
         {{0.2f, EnemyType::Ghoul, 10, false}, {2.6f, EnemyType::Brute, 5, false},
          {5.4f, EnemyType::Bomber, 7, false}}},
        {"warden_sanctum", "WARDEN SANCTUM", "Slay the elite guardian",
         {"warden_sanctum", "WARDEN SANCTUM", MapThree()}, ExpeditionEncounterType::Elite,
         {{0.2f, EnemyType::Brute, 5, false}, {2.5f, EnemyType::Cultist, 7, false},
          {5.0f, EnemyType::GraveWarden, 1, false}}},
        {"stalker_vault", "STALKER VAULT", "Hunt the vault predator",
         {"stalker_vault", "STALKER VAULT", MapStalker()}, ExpeditionEncounterType::Elite,
         {{0.2f, EnemyType::Wraith, 6, false}, {2.7f, EnemyType::Cultist, 6, false},
          {5.2f, EnemyType::VoidStalker, 1, false}}},
        {"broken_confluence", "BROKEN CONFLUENCE", "Survive the converging assault",
         {"broken_confluence", "BROKEN CONFLUENCE", MapFour()}, ExpeditionEncounterType::Combat,
         {{0.2f, EnemyType::Wraith, 7, false}, {2.2f, EnemyType::Necromancer, 3, false},
          {4.8f, EnemyType::ShieldedAcolyte, 7, false}, {7.0f, EnemyType::Bomber, 7, false}}},
        {"ashen_labyrinth", "ASHEN LABYRINTH", "Collapse the ashen host",
         {"ashen_labyrinth", "ASHEN LABYRINTH", MapAshen()}, ExpeditionEncounterType::Combat,
         {{0.2f, EnemyType::Necromancer, 3, false}, {2.4f, EnemyType::Swarmer, 12, false},
          {4.8f, EnemyType::Brute, 6, false}, {7.2f, EnemyType::Bomber, 6, false}}},
        {"crimson_trial", "CRIMSON TRIAL", "Defeat the elite vanguard",
         {"crimson_trial", "CRIMSON TRIAL", MapCrimson()}, ExpeditionEncounterType::Elite,
         {{0.2f, EnemyType::Brute, 4, true}, {2.6f, EnemyType::Cultist, 5, true},
          {5.2f, EnemyType::ShieldedAcolyte, 4, true}}},
        {"void_throne", "VOID THRONE", "Defeat the Void Herald",
         {"void_throne", "VOID THRONE", MapFive()}, ExpeditionEncounterType::Boss, {},
         BossType::VoidHerald}
    };
    return stages;
}

bool ExpeditionDirector::ValidateCatalog(std::string* error) {
    for (const auto& stage : Catalog()) {
        ExpeditionMap map;
        if (!map.Load(stage.map) || !map.Validate(error)) return false;
    }
    return Catalog().size() == 9;
}

void ExpeditionDirector::Reset(std::uint32_t seed) {
    seed_ = seed == 0 ? 1u : seed;
    depth_ = currentCatalogIndex_ = nextGroup_ = 0;
    stageTimer_ = transitionTimer_ = 0.0f;
    introTimer_ = 1.8f;
    phase_ = ExpeditionPhase::Intro;
    rewardWasOpen_ = bossStarted_ = rewardNodeOnly_ = false;
    routeOptions_ = {{-1, -1, -1}};
    routeOptionCount_ = 0;
    currentNodeId_ = -1;
    nodesCompleted_ = 0;
    selectedPath_.clear();
    GenerateRoute();
}

const ExpeditionStageDefinition& ExpeditionDirector::CurrentStage() const {
    return Catalog()[static_cast<std::size_t>(std::clamp(currentCatalogIndex_, 0,
                                                         static_cast<int>(Catalog().size()) - 1))];
}

void ExpeditionDirector::Start(GameSession& session) { LoadNode(session, 0); }

void ExpeditionDirector::GenerateRoute() {
    routeNodes_.clear();
    constexpr std::array<int, RouteLayerCount> counts{{1, 2, 3, 2, 3, 2, 1}};
    std::array<int, RouteLayerCount> starts{};
    std::uint32_t random = seed_;
    auto nextRandom = [&]() {
        random ^= random << 13; random ^= random >> 17; random ^= random << 5;
        return random;
    };
    for (int layer = 0; layer < RouteLayerCount; ++layer) {
        starts[static_cast<std::size_t>(layer)] = static_cast<int>(routeNodes_.size());
        const int specialColumn = (layer == 1 || layer == 3 || layer == 5)
            ? static_cast<int>(nextRandom() % static_cast<std::uint32_t>(counts[layer])) : -1;
        for (int column = 0; column < counts[static_cast<std::size_t>(layer)]; ++column) {
            ExpeditionRouteNode node{};
            node.id = static_cast<int>(routeNodes_.size());
            node.layer = layer;
            node.column = column;
            if (layer == 0) { node.type = ExpeditionEncounterType::Combat; node.catalogIndex = 0; }
            else if (layer == RouteLayerCount - 1) { node.type = ExpeditionEncounterType::Boss; node.catalogIndex = 8; }
            else {
                // Two mandatory combat layers guarantee at least three combat encounters including start.
                if (layer == 2 || layer == 4) node.type = ExpeditionEncounterType::Combat;
                else if (layer == 1 || layer == 5)
                    node.type = column == specialColumn
                                    ? ExpeditionEncounterType::Elite : ExpeditionEncounterType::Combat;
                else
                    node.type = column == specialColumn
                                    ? ExpeditionEncounterType::Reward : ExpeditionEncounterType::Combat;
                if (node.type == ExpeditionEncounterType::Elite)
                    node.catalogIndex = std::array<int, 3>{{3, 4, 7}}[nextRandom() % 3u];
                else {
                    const std::array<int, 2> compatible = layer % 2 == 1
                        ? std::array<int, 2>{{1, 5}} : std::array<int, 2>{{2, 6}};
                    node.catalogIndex = compatible[nextRandom() % 2u];
                }
            }
            routeNodes_.push_back(node);
        }
    }
    for (int layer = 0; layer < RouteLayerCount - 1; ++layer) {
        const int nextStart = starts[static_cast<std::size_t>(layer + 1)];
        const int nextCount = counts[static_cast<std::size_t>(layer + 1)];
        for (int column = 0; column < counts[static_cast<std::size_t>(layer)]; ++column) {
            auto& node = routeNodes_[static_cast<std::size_t>(starts[layer] + column)];
            node.connectionCount = nextCount;
            for (int i = 0; i < nextCount; ++i) node.connections[static_cast<std::size_t>(i)] = nextStart + i;
        }
    }
}

bool ExpeditionDirector::ValidateRoute(std::string* error) const {
    if (routeNodes_.empty() || routeNodes_.front().layer != 0 ||
        routeNodes_.back().type != ExpeditionEncounterType::Boss) {
        if (error) *error = "route endpoints are invalid";
        return false;
    }
    bool elite = false, reward = false;
    for (const auto& node : routeNodes_) {
        elite |= node.type == ExpeditionEncounterType::Elite;
        reward |= node.type == ExpeditionEncounterType::Reward;
        if (node.layer < RouteLayerCount - 1 && node.connectionCount < 1) {
            if (error) *error = "route contains a dead end";
            return false;
        }
        for (int i = 0; i < node.connectionCount; ++i) {
            const int target = node.connections[static_cast<std::size_t>(i)];
            if (target < 0 || target >= static_cast<int>(routeNodes_.size()) ||
                routeNodes_[static_cast<std::size_t>(target)].layer != node.layer + 1) {
                if (error) *error = "route connection is invalid";
                return false;
            }
        }
    }
    if (!elite || !reward) { if (error) *error = "route lacks required choices"; return false; }
    return true;
}

void ExpeditionDirector::LoadNode(GameSession& session, int nodeId) {
    if (nodeId < 0 || nodeId >= static_cast<int>(routeNodes_.size())) return;
    for (auto& node : routeNodes_) if (node.state == ExpeditionRouteNodeState::Current)
        node.state = ExpeditionRouteNodeState::Completed;
    currentNodeId_ = nodeId;
    auto& node = routeNodes_[static_cast<std::size_t>(nodeId)];
    node.state = ExpeditionRouteNodeState::Current;
    selectedPath_.push_back(nodeId);
    depth_ = node.layer;
    if (node.type == ExpeditionEncounterType::Reward) {
        session.ResetStageCombatSystems();
        session.PrepareExpeditionStageReward(true);
        rewardWasOpen_ = rewardNodeOnly_ = true;
        phase_ = ExpeditionPhase::Reward;
        routeOptionCount_ = 0;
        return;
    }
    LoadStage(session, node.catalogIndex, node.layer);
}

void ExpeditionDirector::LoadStage(GameSession& session, int catalogIndex, int depth) {
    currentCatalogIndex_ = std::clamp(catalogIndex, 0, static_cast<int>(Catalog().size()) - 1);
    depth_ = std::clamp(depth, 0, StageCount() - 1);
    session.ResetStageCombatSystems();
    map_.Load(CurrentStage().map);
    map_.SetExitOpen(false);
    session.SetWorldNavigation(&map_);
    session.SetPlayerPosition(map_.Entry());
    nextGroup_ = 0;
    stageTimer_ = 0.0f;
    introTimer_ = 1.8f;
    phase_ = ExpeditionPhase::Intro;
    rewardWasOpen_ = rewardNodeOnly_ = false;
    bossStarted_ = false;
    routeOptionCount_ = 0;
}

void ExpeditionDirector::SpawnGroup(GameSession& session, const ExpeditionSpawnGroup& group) {
    const auto& points = map_.SpawnPoints();
    const float stageScale = 1.0f + depth_ * 0.16f;
    for (int i = 0; i < group.count; ++i) {
        Vector2 point = points[(static_cast<std::size_t>(i + nextGroup_ * 3 + seed_)) % points.size()];
        point.x += static_cast<float>((i % 3) - 1) * 20.0f;
        point.y += static_cast<float>(((i / 3) % 3) - 1) * 20.0f;
        point = map_.FindNearestWalkablePosition(point, GetEnemyDefinition(group.type).radius);
        session.Enemies().Spawn(group.type, point, stageScale * (group.elite ? 0.62f : 1.0f),
                                0.88f + depth_ * 0.08f,
                                1.0f, 2.1f, group.elite, SpawnSource::Director);
    }
}

void ExpeditionDirector::Update(float deltaTime, GameSession& session) {
    session.AdvanceRunTime(deltaTime, std::numeric_limits<float>::max());
    if (phase_ == ExpeditionPhase::Intro) {
        introTimer_ -= deltaTime;
        if (introTimer_ <= 0.0f) phase_ = ExpeditionPhase::Combat;
        return;
    }
    if (phase_ == ExpeditionPhase::Combat) {
        stageTimer_ += deltaTime;
        if (CurrentStage().encounter == ExpeditionEncounterType::Boss) {
            if (!bossStarted_) {
                session.ConfigureBossScaling(0.72f, 0.88f);
                session.Bosses().StartEncounter(CurrentStage().boss, session.GetPlayer().Position());
                bossStarted_ = true;
            }
        } else while (nextGroup_ < static_cast<int>(CurrentStage().groups.size()) &&
                         stageTimer_ >= CurrentStage().groups[static_cast<std::size_t>(nextGroup_)].delay) {
            SpawnGroup(session, CurrentStage().groups[static_cast<std::size_t>(nextGroup_)]);
            ++nextGroup_;
        }
    } else if (phase_ == ExpeditionPhase::Reward) {
        if (rewardWasOpen_ && !session.HasPendingChestReward()) {
            if (rewardNodeOnly_) {
                CompleteCurrentNode();
                PrepareRouteOptions();
                phase_ = ExpeditionPhase::RouteChoice;
            } else {
                phase_ = ExpeditionPhase::Transition;
                transitionTimer_ = 0.0f;
            }
        }
    } else if (phase_ == ExpeditionPhase::ExitOpen) {
        if (DistanceSquared(session.GetPlayer().Position(), map_.Exit()) < 45.0f * 45.0f) {
            phase_ = ExpeditionPhase::Transition;
            transitionTimer_ = 0.0f;
        }
    } else if (phase_ == ExpeditionPhase::Transition) {
        transitionTimer_ += deltaTime;
        if (transitionTimer_ >= 0.85f) {
            CompleteCurrentNode();
            PrepareRouteOptions();
            phase_ = ExpeditionPhase::RouteChoice;
        }
    }
}

int ExpeditionDirector::EnemiesRemaining(const GameSession& session) const {
    int count = 0;
    for (const Enemy& enemy : session.Enemies().Items())
        if (enemy.active && enemy.type != EnemyType::BoneMinion) ++count;
    if (session.Bosses().IsActive()) ++count;
    return count;
}

void ExpeditionDirector::BeginReward(GameSession& session) {
    if (phase_ != ExpeditionPhase::Combat) return;
    session.GetPlayer().Heal((session.GetPlayer().stats.maxHP - session.GetPlayer().stats.currentHP) * 0.12f);
    session.Enemies().DespawnBySource(SpawnSource::Necromancer);
    session.Projectiles().ClearOwner(ProjectileOwner::Enemy);
    session.PrepareExpeditionStageReward(CurrentStage().encounter == ExpeditionEncounterType::Elite);
    rewardWasOpen_ = true;
    phase_ = ExpeditionPhase::Reward;
}

void ExpeditionDirector::AfterSharedUpdate(GameSession& session) {
    if (phase_ != ExpeditionPhase::Combat) return;
    if (CurrentStage().encounter == ExpeditionEncounterType::Boss) {
        if (bossStarted_ && session.HasBossDeathThisFrame()) {
            CompleteCurrentNode();
            phase_ = ExpeditionPhase::Complete;
        }
    } else if (nextGroup_ >= static_cast<int>(CurrentStage().groups.size()) &&
               EnemiesRemaining(session) == 0) BeginReward(session);
}

float ExpeditionDirector::TransitionAlpha() const {
    if (phase_ != ExpeditionPhase::Transition) return 0.0f;
    return std::clamp(transitionTimer_ / 0.85f, 0.0f, 1.0f);
}

void ExpeditionDirector::DebugCompleteStage(GameSession& session) {
    if (CurrentStage().encounter == ExpeditionEncounterType::Boss) {
        if (!session.Bosses().IsActive()) { bossStarted_ = false; phase_ = ExpeditionPhase::Combat; Update(0.0f, session); }
        session.DebugDamageBoss(2.0f);
    } else { session.DebugKillAll(); nextGroup_ = static_cast<int>(CurrentStage().groups.size()); }
}
void ExpeditionDirector::DebugOpenReward(GameSession& session) {
    if (depth_ < StageCount() - 1) {
        if (phase_ == ExpeditionPhase::Intro) phase_ = ExpeditionPhase::Combat;
        BeginReward(session);
    }
}
void ExpeditionDirector::DebugGoToBoss(GameSession& session) {
    LoadNode(session, static_cast<int>(routeNodes_.size()) - 1);
}

void ExpeditionDirector::PrepareRouteOptions() {
    routeOptions_ = {{-1, -1, -1}};
    routeOptionCount_ = 0;
    if (currentNodeId_ < 0 || currentNodeId_ >= static_cast<int>(routeNodes_.size())) return;
    const auto& current = routeNodes_[static_cast<std::size_t>(currentNodeId_)];
    for (int i = 0; i < current.connectionCount; ++i) {
        const int id = current.connections[static_cast<std::size_t>(i)];
        routeOptions_[static_cast<std::size_t>(routeOptionCount_++)] = id;
        routeNodes_[static_cast<std::size_t>(id)].state = ExpeditionRouteNodeState::Available;
    }
}

const ExpeditionStageDefinition& ExpeditionDirector::RouteOption(int index) const {
    const int safe = std::clamp(index, 0, std::max(0, routeOptionCount_ - 1));
    const auto& node = routeNodes_[static_cast<std::size_t>(routeOptions_[static_cast<std::size_t>(safe)])];
    return Catalog()[static_cast<std::size_t>(node.catalogIndex)];
}

const ExpeditionRouteNode& ExpeditionDirector::RouteOptionNode(int index) const {
    const int safe = std::clamp(index, 0, std::max(0, routeOptionCount_ - 1));
    return routeNodes_[static_cast<std::size_t>(routeOptions_[static_cast<std::size_t>(safe)])];
}

bool ExpeditionDirector::SelectRoute(int option, GameSession& session) {
    if (phase_ != ExpeditionPhase::RouteChoice || option < 0 || option >= routeOptionCount_) return false;
    const int nodeId = routeOptions_[static_cast<std::size_t>(option)];
    for (int i = 0; i < routeOptionCount_; ++i) {
        const int id = routeOptions_[static_cast<std::size_t>(i)];
        if (id != nodeId) routeNodes_[static_cast<std::size_t>(id)].state = ExpeditionRouteNodeState::Unavailable;
    }
    LoadNode(session, nodeId);
    return true;
}

void ExpeditionDirector::CompleteCurrentNode() {
    if (currentNodeId_ < 0) return;
    auto& node = routeNodes_[static_cast<std::size_t>(currentNodeId_)];
    if (node.state != ExpeditionRouteNodeState::Completed) ++nodesCompleted_;
    node.state = ExpeditionRouteNodeState::Completed;
}
