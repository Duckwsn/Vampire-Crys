#include <cmath>
#include <chrono>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <limits>
#include <memory>
#include <set>

#include "core/GameSettings.h"
#include "core/GameFlow.h"
#include "entities/Player.h"
#include "gameplay/Definitions.h"
#include "gameplay/GameSession.h"
#include "game_modes/GameModeManager.h"
#include "game_modes/ExpeditionDirector.h"
#include "game_modes/WorldNavigation.h"
#include "game_modes/SurvivalEndgame.h"
#include "game_modes/SurvivalEvents.h"
#include "systems/AreaEffectManager.h"
#include "systems/BossManager.h"
#include "systems/EnemyManager.h"
#include "systems/FloatingTextManager.h"
#include "systems/ParticleManager.h"
#include "systems/ProjectileManager.h"
#include "systems/ScreenShakeManager.h"
#include "systems/TelegraphManager.h"
#include "systems/WeaponManager.h"
#include "systems/WaveDirector.h"
#include "systems/XPOrbManager.h"
#include "ui/VisualStyle.h"
#include "ui/Localization.h"

namespace {
int failures = 0;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++failures;
    }
}

bool Near(float left, float right, float epsilon = 0.001f) {
    return std::fabs(left - right) <= epsilon;
}
} // namespace

int main() {
    Check(EscapeActionFor(GameState::Playing) == EscapeAction::OpenPause &&
              EscapeActionFor(GameState::Paused) == EscapeAction::Resume,
          "escape maps Playing and Paused to exactly one opposite-state transition");
    Check(EscapeActionFor(GameState::LevelUp) == EscapeAction::None &&
              EscapeActionFor(GameState::ChestReward) == EscapeAction::None,
          "escape does not dismiss mandatory reward states");
    Check(EscapeActionFor(GameState::ModeSelection) == EscapeAction::ReturnToMainMenu,
          "mode selection returns to main menu with Escape");
    Check(EscapeActionFor(GameState::CharacterSelection) == EscapeAction::ReturnToModeSelection,
          "character selection returns to mode selection with Escape");
    Check(ShouldUpdateGameplay(GameState::Playing) &&
              !ShouldUpdateGameplay(GameState::Paused) &&
              !ShouldUpdateGameplay(GameState::Settings),
          "only Playing advances gameplay simulation");
    Check(EscapeActionFor(GameState::SurvivalSetup) == EscapeAction::ReturnToCharacterSelection,
          "Survival setup returns to character selection with Escape");

    {
        SurvivalRunConfig standard{};
        const RunModifiers normal = BuildRunModifiers(standard);
        Check(standard.difficulty == DifficultyTier::Normal && standard.ascension == 0 &&
                  !standard.endless && standard.challenge == ChallengeId::None &&
                  standard.MutatorCount() == 0 && Near(normal.enemyHp, 1.0f) &&
                  Near(normal.enemyDamage, 1.0f) && Near(normal.spawnInterval, 1.0f),
              "quick-start config preserves the original Normal A0 Survival balance");

        SurvivalRunConfig hard{};
        hard.difficulty = DifficultyTier::Hard;
        SurvivalRunConfig nightmare{};
        nightmare.difficulty = DifficultyTier::Nightmare;
        const RunModifiers hardMods = BuildRunModifiers(hard);
        const RunModifiers nightmareMods = BuildRunModifiers(nightmare);
        Check(hardMods.enemyHp > normal.enemyHp && nightmareMods.enemyHp > hardMods.enemyHp &&
                  hardMods.enemyDamage > normal.enemyDamage && nightmareMods.enemyDamage > hardMods.enemyDamage &&
                  hardMods.spawnInterval < normal.spawnInterval && nightmareMods.spawnInterval < hardMods.spawnInterval,
              "Normal, Hard and Nightmare apply ordered enemy health, damage and pressure");

        SurvivalRunConfig ascension{};
        ascension.ascension = 10;
        const RunModifiers asc10 = BuildRunModifiers(ascension);
        Check(asc10.enemyHp > normal.enemyHp && asc10.enemyDamage > normal.enemyDamage &&
                  asc10.spawnPressure > normal.spawnPressure && asc10.eventInterval < normal.eventInterval &&
                  asc10.specialWaveInterval < normal.specialWaveInterval,
              "Ascension 10 and its milestones increase several independent pressures");
        ascension.ascension = 99;
        ascension.Sanitize();
        Check(ascension.ascension == 10, "Ascension is safely clamped to the supported 0-10 range");

        std::array<bool, static_cast<std::size_t>(MutatorId::Count)> hasEffect{};
        for (int raw = 0; raw < static_cast<int>(MutatorId::Count); ++raw) {
            SurvivalRunConfig single{};
            const MutatorId id = static_cast<MutatorId>(raw);
            Check(single.ToggleMutator(id), "each individual mutator can be selected in Standard setup");
            const RunModifiers modified = BuildRunModifiers(single);
            hasEffect[static_cast<std::size_t>(raw)] =
                !Near(modified.enemyHp, normal.enemyHp) || !Near(modified.enemySpeed, normal.enemySpeed) ||
                !Near(modified.spawnInterval, normal.spawnInterval) || !Near(modified.spawnPressure, normal.spawnPressure) ||
                !Near(modified.eliteChance, normal.eliteChance) || !Near(modified.playerDamage, normal.playerDamage) ||
                !Near(modified.playerHp, normal.playerHp) || !Near(modified.playerSpeed, normal.playerSpeed) ||
                !Near(modified.healthDropChance, normal.healthDropChance) ||
                !Near(modified.eventInterval, normal.eventInterval) ||
                !Near(modified.specialWaveInterval, normal.specialWaveInterval);
            Check(hasEffect[static_cast<std::size_t>(raw)] && modified.scoreMultiplier > 1.0f,
                  "every mutator changes gameplay and increases the score multiplier");
        }

        SurvivalRunConfig selection{};
        Check(selection.ToggleMutator(MutatorId::HyperHorde) &&
                  !selection.ToggleMutator(MutatorId::Titanic) &&
                  selection.ToggleMutator(MutatorId::Eventful) &&
                  selection.ToggleMutator(MutatorId::BloodRush) &&
                  !selection.ToggleMutator(MutatorId::ScarceRecovery) && selection.MutatorCount() == 3,
              "setup enforces Hyper/Titanic incompatibility and the three-mutator limit");

        for (int raw = 1; raw < static_cast<int>(ChallengeId::Count); ++raw) {
            SurvivalRunConfig challenge{};
            challenge.challenge = static_cast<ChallengeId>(raw);
            challenge.difficulty = DifficultyTier::Normal;
            challenge.ascension = 0;
            challenge.endless = true;
            SurvivalRunConfig resolved = ResolveSurvivalConfig(challenge);
            const ChallengeDefinition& definition = GetChallengeDefinition(challenge.challenge);
            Check(resolved.difficulty == definition.difficulty && resolved.ascension == definition.ascension &&
                      resolved.mutatorMask == definition.mutatorMask && !resolved.endless,
                  "each predefined challenge resolves to its locked rules and disables Endless");
            Check(!resolved.ToggleMutator(MutatorId::BloodRush),
                  "challenge configurations cannot be edited through mutator toggles");
        }
        SurvivalRunConfig swarm{}; swarm.challenge = ChallengeId::TheSwarm;
        SurvivalRunConfig titans{}; titans.challenge = ChallengeId::Titans;
        SurvivalRunConfig moon{}; moon.challenge = ChallengeId::BloodMoon;
        SurvivalRunConfig hunter{}; hunter.challenge = ChallengeId::BossHunter;
        Check(BuildRunModifiers(swarm).forceSwarm && BuildRunModifiers(titans).forceDurable &&
                  BuildRunModifiers(moon).preferBloodMoon && BuildRunModifiers(hunter).bossHunter,
              "challenge-specific swarm, durable, Blood Moon and boss rules are active");

        SurvivalRunConfig extreme{};
        extreme.difficulty = DifficultyTier::Nightmare;
        extreme.ascension = 10;
        extreme.endless = true;
        extreme.ToggleMutator(MutatorId::HyperHorde);
        extreme.ToggleMutator(MutatorId::Eventful);
        extreme.ToggleMutator(MutatorId::ChaoticWaves);
        const RunModifiers cycle5 = BuildRunModifiers(extreme, 5);
        const RunModifiers cycle99 = BuildRunModifiers(extreme, 99);
        Check(std::isfinite(cycle5.enemyHp) && std::isfinite(cycle99.enemyHp) &&
                  cycle99.spawnInterval >= 0.32f && cycle99.enemySpeed <= 1.25f &&
                  cycle99.scoreMultiplier <= 8.0f && cycle99.enemyHp >= cycle5.enemyHp,
              "Nightmare A10 triple-mutator Endless scaling remains finite and safely capped");

        const ScoreBreakdown baseScore = CalculateSurvivalScore(100, 5, 2, 1, 600.0f, standard);
        const ScoreBreakdown hardScore = CalculateSurvivalScore(100, 5, 2, 1, 600.0f, hard);
        const ScoreBreakdown extremeScore = CalculateSurvivalScore(100, 5, 2, 1, 600.0f, extreme);
        Check(baseScore.baseScore == 4375 && baseScore.finalScore == 4375 &&
                  hardScore.finalScore > baseScore.finalScore && extremeScore.finalScore > hardScore.finalScore &&
                  extremeScore.difficultyMultiplier > 1.0f && extremeScore.ascensionMultiplier > 1.0f &&
                  extremeScore.mutatorMultiplier > 1.0f,
              "score breakdown is deterministic and rewards difficulty, Ascension and mutators");
    }
    Check(VisualStyle::EnvironmentHalfCoverage(960) == 640 &&
              VisualStyle::EnvironmentHalfCoverage(1280) == 800 &&
              VisualStyle::EnvironmentHalfCoverage(1920) == 1120 &&
              VisualStyle::ViewportFadeWidth == 48,
          "environment coverage follows resized viewport with a bounded edge fade");
    {
        auto modeSession = std::make_unique<GameSession>();
        auto modes = std::make_unique<GameModeManager>();
        std::string mapError;
        Check(ExpeditionDirector::ValidateCatalog(&mapError),
              "all authored Expedition maps have connected entry, exit and spawn zones");
        std::string firstFingerprint;
        std::set<std::string> routeFingerprints;
        for (unsigned int seed = 1; seed <= 100; ++seed) {
            ExpeditionDirector route;
            route.Reset(seed);
            std::string routeError;
            Check(route.ValidateRoute(&routeError),
                  "seeded Expedition route is connected and has no dead ends");
            std::string fingerprint;
            for (const auto& node : route.RouteNodes())
                fingerprint += std::to_string(node.layer) + ":" +
                               std::to_string(static_cast<int>(node.type)) + ":" +
                               std::to_string(node.catalogIndex) + "|";
            routeFingerprints.insert(fingerprint);
            if (seed == 1) firstFingerprint = fingerprint;
        }
        ExpeditionDirector repeatedRoute;
        repeatedRoute.Reset(1);
        std::string repeatedFingerprint;
        for (const auto& node : repeatedRoute.RouteNodes())
            repeatedFingerprint += std::to_string(node.layer) + ":" +
                                   std::to_string(static_cast<int>(node.type)) + ":" +
                                   std::to_string(node.catalogIndex) + "|";
        Check(repeatedFingerprint == firstFingerprint,
              "the same Expedition seed reproduces the same route");
        Check(routeFingerprints.size() > 20,
              "different Expedition seeds produce meaningful route variety");
        for (const ExpeditionStageDefinition& stage : ExpeditionDirector::Catalog()) {
            ExpeditionMap map;
            Check(map.Load(stage.map) && map.IsWalkable(map.Entry(), 18.0f) &&
                      map.IsWalkable(map.Exit(), 18.0f),
                  "each Expedition map exposes radius-safe entry and exit points");
            const Vector2 escaped{map.Bounds().x - 500.0f, map.Bounds().y - 500.0f};
            const Vector2 constrained = map.ConstrainMovement(map.Entry(), escaped, 18.0f);
            Check(map.IsWalkable(constrained, 18.0f),
                  "map collision rejects movement into void without teleporting off footprint");
            const Vector2 camera = map.ConstrainCamera(map.Entry(), {640.0f, 360.0f});
            Check(std::isfinite(camera.x) && std::isfinite(camera.y),
                  "camera constraints remain finite on every authored viewport");
            const Vector2 direction = map.DirectionToward(map.SpawnPoints()[0], map.Entry());
            Check(std::isfinite(direction.x) && std::isfinite(direction.y),
                  "flow navigation provides a finite route from every stage spawn region");
            EnemyManager guardian;
            ProjectileManager guardianShots;
            guardian.SpawnMiniboss(EnemyType::GraveWarden,
                                   map.FindNearestWalkablePosition(map.Entry(), 42.0f));
            for (int step = 0; step < 100; ++step)
                guardian.Update(0.05f, map.Exit(), guardianShots, 1.0f, &map);
            const Enemy* activeGuardian = guardian.ActiveMiniboss();
            Check(activeGuardian != nullptr && map.IsWalkable(activeGuardian->position,
                                                               activeGuardian->radius),
                  "Grave Warden chase and charge remain inside every authored arena");
        }
        Check(modes->SelectMode(GameModeType::Expedition) && !modes->HasActiveMode(),
              "Expedition is selectable without activating before character choice");
        Check(modes->ActivateMode(GameModeType::Survival, *modeSession) &&
                  modes->HasActiveMode() && modes->ActiveType() == GameModeType::Survival &&
                  modes->HUDData().duration == 900.0f,
              "Survival mode activates with its own progression and HUD data");
        modes->DebugForceEvent(*modeSession);
        modes->UpdateActiveMode(0.0f, *modeSession);
        Check(modes->HUDData().eventActive,
              "debug event trigger enters the Survival event lifecycle");
        modeSession->GrantDebugXP(100);
        modes->DebugSpawnEnemies(8, *modeSession);
        Check(modes->RestartActiveMode(*modeSession) && modeSession->GetPlayer().stats.level == 1 &&
                  modeSession->Enemies().ActiveCount() == 0 &&
                  modes->Context().outcome == RunOutcome::None &&
                  !modes->HUDData().eventActive &&
                  std::string(modes->HUDData().specialWaveName) == "NONE" &&
                  Near(modeSession->GetPlayer().DashCooldownRemaining(), 0.0f),
              "restart resets shared and Survival-specific state in the current mode");
        modes->DebugForceEvent(*modeSession);
        modes->UpdateActiveMode(0.0f, *modeSession);
        modeSession->GetPlayer().TakeDamage(10000.0f);
        Check(modes->UpdateActiveMode(0.01f, *modeSession) == RunOutcome::Defeat,
              "death during an event resolves through the normal Game Over outcome");
        Check(modes->RestartActiveMode(*modeSession) && !modes->HUDData().eventActive,
              "restart after event death clears all temporary event state");
        modes->ExitActiveMode(*modeSession);
        Check(!modes->HasActiveMode() && modeSession->Enemies().ActiveCount() == 0,
              "leaving a mode clears run state before returning to the menu");
        ExpeditionRunConfig expeditionConfig{424242u};
        Check(modes->ActivateMode(GameModeType::Expedition, *modeSession, CharacterId::Sentinel,
                                  {}, expeditionConfig) &&
                  modes->HUDData().stageCount == 7 && modes->HUDData().progressIndex == 1 &&
                  modeSession->Navigation() != nullptr,
              "Expedition starts at authored Stage 1 with world navigation attached");
        const Vector2 expeditionStart = modeSession->GetPlayer().Position();
        Check(modeSession->Navigation()->IsWalkable(expeditionStart, modeSession->GetPlayer().Radius()),
              "Expedition places the player on a safe walkable entry cell");
        Check(modes->RestartActiveMode(*modeSession) &&
                  modes->Context().expeditionConfig.seed == 424242u &&
                  modes->HUDData().progressIndex == 1 && modeSession->GetPlayer().stats.level == 1,
              "Expedition restart preserves seed and character while rebuilding a clean Stage 1 run");
        modes->DebugOpenReward(*modeSession);
        Check(modeSession->HasPendingChestReward() && modeSession->ChestChoiceCount() == 3,
              "Expedition encounter reward always provides three valid choices");
        modeSession->SelectChestReward(0);
        modes->UpdateActiveMode(0.0f, *modeSession);
        modes->UpdateActiveMode(0.9f, *modeSession);
        Check(modes->HUDData().routeChoiceActive,
              "selecting the stage reward advances to the generated route map");
        modes->DebugGoToBoss(*modeSession);
        Check(modes->HUDData().progressIndex == 7 && modeSession->Navigation() != nullptr,
              "Expedition debug navigation loads the final authored boss stage safely");
        modes->DebugCompleteStage(*modeSession);
        Check(modes->UpdateActiveMode(0.0f, *modeSession) == RunOutcome::Victory,
              "defeating the Expedition final boss resolves the shared Victory outcome");
    }

    {
        SurvivalRunConfig glass{};
        glass.ToggleMutator(MutatorId::GlassCannon);
        auto glassSession = std::make_unique<GameSession>();
        auto glassModes = std::make_unique<GameModeManager>();
        glassModes->ActivateMode(GameModeType::Survival, *glassSession, CharacterId::Occultist, glass);
        const float firstGlassHP = glassSession->GetPlayer().stats.maxHP;
        Check(firstGlassHP >= 20.0f && firstGlassHP < 60.0f &&
                  glassSession->SurvivalConfig().HasMutator(MutatorId::GlassCannon),
              "Glass Cannon composes safely with the lowest-HP character");
        glassSession->GrantDebugXP(500);
        glassModes->RestartActiveMode(*glassSession);
        Check(Near(glassSession->GetPlayer().stats.maxHP, firstGlassHP) &&
                  glassSession->GetPlayer().stats.level == 1 &&
                  glassModes->Context().survivalConfig.HasMutator(MutatorId::GlassCannon),
              "restart preserves character and immutable run config without stacking modifiers");
    }

    {
        auto scoreSession = std::make_unique<GameSession>();
        scoreSession->Reset();
        scoreSession->Enemies().Spawn(EnemyType::BoneMinion, {100.0f, 0.0f}, 1.0f, 1.0f, 1.0f,
                                      1.0f, false, SpawnSource::Necromancer);
        scoreSession->DebugKillAll();
        Check(scoreSession->Statistics().enemiesKilled == 0 &&
                  scoreSession->Statistics().scoreEligibleKills == 0,
              "summoned minions are excluded from score-eligible kill accounting");
        scoreSession->Enemies().Spawn(EnemyType::Ghoul, {100.0f, 0.0f});
        scoreSession->DebugKillAll();
        Check(scoreSession->Statistics().scoreEligibleKills == 1 &&
                  scoreSession->CurrentScore().baseScore == 10,
              "director enemies add to deterministic score accounting");
    }

    {
        SurvivalRunConfig hard{};
        hard.difficulty = DifficultyTier::Hard;
        auto hardSession = std::make_unique<GameSession>();
        auto hardModes = std::make_unique<GameModeManager>();
        hardModes->ActivateMode(GameModeType::Survival, *hardSession, CharacterId::Hunter, hard);
        hardSession->GetPlayer().ToggleDebugInvulnerability();
        hardModes->DebugSkipTime(300.0f, *hardSession);
        hardModes->UpdateActiveMode(0.0f, *hardSession);
        const float hardBossHP = hardSession->Bosses().IsActive()
                                     ? hardSession->Bosses().ActiveBoss().maxHP : 0.0f;

        SurvivalRunConfig nightmare{};
        nightmare.difficulty = DifficultyTier::Nightmare;
        auto nightmareSession = std::make_unique<GameSession>();
        auto nightmareModes = std::make_unique<GameModeManager>();
        nightmareModes->ActivateMode(GameModeType::Survival, *nightmareSession,
                                     CharacterId::Hunter, nightmare);
        nightmareSession->GetPlayer().ToggleDebugInvulnerability();
        nightmareModes->DebugSkipTime(300.0f, *nightmareSession);
        nightmareModes->UpdateActiveMode(0.0f, *nightmareSession);
        const float nightmareBossHP = nightmareSession->Bosses().IsActive()
                                          ? nightmareSession->Bosses().ActiveBoss().maxHP : 0.0f;
        Check(hardBossHP > 4200.0f && nightmareBossHP > hardBossHP,
              "difficulty multipliers reach actual scheduled boss health");
    }

    {
        SurvivalRunConfig endless{};
        endless.endless = true;
        endless.ascension = 3;
        endless.ToggleMutator(MutatorId::Eventful);
        auto session = std::make_unique<GameSession>();
        auto modes = std::make_unique<GameModeManager>();
        modes->ActivateMode(GameModeType::Survival, *session, CharacterId::Sentinel, endless);
        session->GetPlayer().ToggleDebugInvulnerability();

        session->DebugSpawnBoss(BossType::FlameWyrm);
        session->DebugDamageBoss(1.1f);
        modes->UpdateActiveMode(0.0f, *session);
        session->DebugSpawnBoss(BossType::VoidHerald);
        session->DebugDamageBoss(1.1f);
        modes->UpdateActiveMode(0.0f, *session);
        session->DebugSpawnBoss(BossType::VoidHeraldAscended);
        modes->DebugSkipTime(900.0f, *session);
        modes->UpdateActiveMode(0.0f, *session);
        const bool finalSpawned = session->Bosses().IsActive() &&
                                  session->Bosses().ActiveBoss().type == BossType::VoidHeraldAscended;
        session->DebugDamageBoss(1.1f);
        const RunOutcome afterFinal = modes->UpdateActiveMode(0.0f, *session);
        Check(finalSpawned && afterFinal == RunOutcome::None && modes->HasActiveMode() &&
                  modes->Context().outcome == RunOutcome::None,
              "Endless defeats the 15-minute final boss without ending the run");

        modes->DebugSkipTime(300.0f, *session);
        modes->UpdateActiveMode(2.0f, *session);
        modes->UpdateActiveMode(0.0f, *session);
        const bool cycleBoss = session->Bosses().IsActive() &&
                               session->Bosses().ActiveBoss().type == BossType::FlameWyrm;
        Check(cycleBoss && modes->HUDData().endless && modes->HUDData().endlessCycle == 2 &&
                  session->EndlessCyclesCompleted() == 1,
              "Endless cycle two schedules the next rotating boss and updates HUD progress");
        session->DebugDamageBoss(1.1f);
        modes->UpdateActiveMode(0.0f, *session);
        modes->DebugSkipTime(300.0f, *session);
        modes->UpdateActiveMode(2.0f, *session);
        modes->UpdateActiveMode(0.0f, *session);
        Check(session->Bosses().IsActive() && session->Bosses().ActiveBoss().type == BossType::VoidHerald &&
                  modes->Context().outcome == RunOutcome::None,
              "Endless rotates bosses every five minutes without producing Victory");

        modes->RestartActiveMode(*session);
        Check(Near(session->ElapsedTime(), 0.0f) && !session->Bosses().IsActive() &&
                  session->Enemies().ActiveCount() == 0 && session->EndlessCyclesCompleted() == 0 &&
                  modes->HUDData().endless && modes->HUDData().endlessCycle == 1 &&
                  modes->Context().character == CharacterId::Sentinel &&
                  modes->Context().survivalConfig.ascension == 3,
              "Endless restart starts a clean run with the same character and configuration");
        modes->DebugSkipTime(25000.0f, *session);
        Check(session->ElapsedTime() > 21600.0f,
              "Endless timer, scheduling and time score do not freeze at a hidden six-hour cap");
        modes->RestartActiveMode(*session);
        modes->DebugSkipTime(901.0f, *session);
        session->GetPlayer().TakeDamage(10000.0f);
        Check(modes->UpdateActiveMode(0.0f, *session) == RunOutcome::Defeat &&
                  modes->Result().outcome == RunOutcome::Defeat && !modes->Result().victory &&
                  modes->Result().survivalConfig.endless && modes->Result().endlessCycles == 1 &&
                  modes->Result().character == CharacterId::Sentinel &&
                  modes->Result().score.finalScore >= modes->Result().score.baseScore,
              "Endless death produces a complete defeat result with config, cycles and score");
    }

    {
        std::set<int> startingWeapons;
        for (int raw = 0; raw < static_cast<int>(CharacterId::Count); ++raw) {
            const CharacterDefinition& definition = GetCharacterDefinition(static_cast<CharacterId>(raw));
            startingWeapons.insert(static_cast<int>(definition.startingWeapon));
            Check(definition.name[0] != '\0' && definition.traitName[0] != '\0' &&
                      definition.traitDescription[0] != '\0',
                  "each playable character exposes complete selection metadata");
        }
        Check(startingWeapons.size() == 4, "four characters have four distinct starting weapons");

        auto characterSession = std::make_unique<GameSession>();
        auto characterModes = std::make_unique<GameModeManager>();
        for (int raw = 0; raw < static_cast<int>(CharacterId::Count); ++raw) {
            const CharacterId character = static_cast<CharacterId>(raw);
            characterModes->ActivateMode(GameModeType::Survival, *characterSession, character);
            Check(characterSession->GetPlayer().Character() == character &&
                      characterSession->Loadout().WeaponCount() == 1 &&
                      characterSession->Loadout().HasWeapon(GetCharacterDefinition(character).startingWeapon),
                  "selected character reaches Survival with exactly its starting weapon");
            const float expectedHP = characterSession->GetPlayer().stats.maxHP;
            const float expectedSpeed = characterSession->GetPlayer().stats.moveSpeed;
            characterSession->GrantDebugXP(500);
            characterSession->DebugGrantWeapon(WeaponType::SoulScythe);
            characterModes->RestartActiveMode(*characterSession);
            Check(characterModes->Context().character == character &&
                      characterSession->GetPlayer().Character() == character &&
                      Near(characterSession->GetPlayer().stats.maxHP, expectedHP) &&
                      Near(characterSession->GetPlayer().stats.moveSpeed, expectedSpeed) &&
                      characterSession->GetPlayer().stats.level == 1 &&
                      characterSession->Loadout().WeaponCount() == 1,
                  "restart preserves character identity and clears run upgrades without stat leakage");
        }
        characterModes->ActivateMode(GameModeType::Survival, *characterSession, CharacterId::Hunter);
        Check(Near(characterSession->GetPlayer().stats.maxHP, 100.0f) &&
                  Near(characterSession->GetPlayer().stats.moveSpeed, 235.0f),
              "returning to Hunter restores clean baseline stats after other characters");

        Player pyro;
        pyro.Reset(CharacterId::Pyromancer);
        pyro.RegisterAoEHits(5);
        Check(pyro.TraitTimer() > 2.9f && pyro.stats.temporaryAreaMultiplier > 1.1f,
              "Pyromancer gains its bounded temporary area trait from AoE hits");
        pyro.Update(3.1f, {-2000, -2000, 4000, 4000});
        Check(Near(pyro.stats.temporaryAreaMultiplier, 1.0f),
              "Pyromancer temporary area trait expires without permanent stat changes");

        Player sentinel;
        sentinel.Reset(CharacterId::Sentinel);
        sentinel.Update(5.1f, {-2000, -2000, 4000, 4000});
        const float sentinelBefore = sentinel.stats.currentHP;
        Check(sentinel.FortifiedReady() && sentinel.TakeDamage(22.0f) &&
                  Near(sentinelBefore - sentinel.stats.currentHP, 12.3f, 0.01f) && !sentinel.FortifiedReady(),
              "Sentinel Fortified reduces one hit after five unharmed seconds and is consumed");

        Player occultist;
        occultist.Reset(CharacterId::Occultist);
        Check(occultist.stats.maxHP < 100.0f && occultist.stats.luck >= 0.35f &&
                  occultist.stats.criticalChance >= 0.09f,
              "Occultist applies its explicit HP tradeoff, Luck and critical bonuses once");

        EnemyManager hunterTargets;
        hunterTargets.Spawn(EnemyType::Brute, {100.0f, 0.0f});
        const float hunterHP = hunterTargets.Items()[0].hp;
        hunterTargets.SetPlayerDamageBonuses(0.15f, 0.0f);
        hunterTargets.Damage(0, 10.0f, {}, 0.0f, DamageSource::PlayerProjectile);
        Check(Near(hunterHP - hunterTargets.Items()[0].hp, 11.5f),
              "Hunter Marked Prey increases damage against healthy normal enemies");
    }

    {
        Player dasher;
        dasher.Reset();
        Check(dasher.TryPhaseDash({1.0f, 0.0f}) && dasher.IsDashing() &&
                  !dasher.TakeDamage(20.0f),
              "Phase Dash starts on demand and grants brief invulnerability");
        Check(!dasher.TryPhaseDash({0.0f, 1.0f}), "Phase Dash cannot restart during cooldown");
        dasher.Update(0.21f, {-2000, -2000, 4000, 4000});
        Check(!dasher.IsDashing() && dasher.TakeDamage(20.0f) &&
                  dasher.DashCooldownRemaining() > 7.0f,
              "Phase Dash invulnerability ends while its cooldown remains active");
        dasher.Reset();
        Check(Near(dasher.DashCooldownRemaining(), 0.0f), "Phase Dash resets for a clean run");
    }

    {
        std::set<SurvivalEventType> observed;
        EnemyManager enemies;
        for (unsigned int seed = 1; seed <= 80 && observed.size() < 3; ++seed) {
            SurvivalEventManager events;
            events.Reset(seed);
            events.ForceNext();
            events.Update(0.0f, 100.0f, false, {}, enemies);
            Check(events.State() == SurvivalEventState::Telegraph,
                  "Survival event enters a telegraph state before activation");
            events.Update(3.1f, 103.1f, false, {}, enemies);
            observed.insert(events.Type());
            const SurvivalEventModifiers modifiers = events.Modifiers();
            Check(events.State() == SurvivalEventState::Active &&
                      (modifiers.spawnIntervalMultiplier != 1.0f ||
                       events.Type() == SurvivalEventType::EliteHunt),
                  "Survival event activates a scoped runtime modifier");
        }
        Check(observed.count(SurvivalEventType::BloodMoon) &&
                  observed.count(SurvivalEventType::TheSwarm) &&
                  observed.count(SurvivalEventType::EliteHunt),
              "Blood Moon, The Swarm and Elite Hunt are all selectable");
        SurvivalEventManager blocked;
        blocked.Reset(7); blocked.ForceNext();
        blocked.Update(0.0f, 100.0f, true, {}, enemies);
        Check(!blocked.IsActive(), "events do not start while a boss or special encounter blocks them");
    }

    {
        for (int raw = 1; raw < static_cast<int>(SpecialWaveType::Count); ++raw) {
            WaveDirector director;
            EnemyManager enemies;
            ProjectileManager shots;
            director.Reset();
            director.ForceSpecialWave(static_cast<SpecialWaveType>(raw));
            director.Update(0.01f, 100.0f, {}, 1.0f, enemies);
            Check(director.ActiveSpecialWave() == static_cast<SpecialWaveType>(raw) &&
                      enemies.ActiveCount() > 0 && director.ThreatBudget() >= 0.0f,
                  "each special wave starts through the threat-budget director");
        }
    }

    {
        ProjectileManager shots;
        EnemyManager enemies;
        enemies.Spawn(EnemyType::ShieldedAcolyte, {});
        const float shieldedHP = enemies.Items()[0].hp;
        enemies.Damage(0, 40.0f, {}, 0.0f, DamageSource::PlayerProjectile);
        Check(shieldedHP - enemies.Items()[0].hp < 40.0f && enemies.Items()[0].shieldHP >= 0.0f,
              "Shielded Acolyte absorbs part of incoming damage with a breakable shield");

        enemies.Reset();
        enemies.Spawn(EnemyType::Wraith, {});
        enemies.Update(2.2f, {500.0f, 0.0f}, shots);
        Check(enemies.Items()[0].intangible &&
                  !enemies.Damage(0, 50.0f, {}, 0.0f, DamageSource::PlayerProjectile),
              "Wraith phase is intangible and disables contact damage");

        enemies.Reset();
        enemies.Spawn(EnemyType::Necromancer, {350.0f, 0.0f});
        enemies.Update(5.0f, {}, shots);
        Check(enemies.ActiveCount(EnemyType::BoneMinion) > 0 &&
                  enemies.ActiveCount(EnemyType::BoneMinion) <= 3,
              "Necromancer summons capped low-value Bone Minions");

        enemies.Reset();
        enemies.Spawn(EnemyType::Ghoul, {}, 1, 1, 1, 1, false,
                      SpawnSource::Director, EnemyVariant::Armored);
        Check(enemies.VariantCount() == 1 && enemies.Items()[0].maxHP > GetEnemyDefinition(EnemyType::Ghoul).maxHP,
              "enemy variants apply bounded spawn-time modifiers without becoming elites");
    }

    {
        ProjectileManager shots;
        EnemyManager minibosses;
        Check(minibosses.SpawnMiniboss(EnemyType::GraveWarden, {100.0f, 0.0f}),
              "Grave Warden uses the shared enemy pool");
        minibosses.Update(0.1f, {}, shots);
        minibosses.Update(1.0f, {}, shots);
        minibosses.Update(0.5f, {}, shots);
        minibosses.Update(0.7f, {}, shots);
        minibosses.Update(1.9f, {}, shots);
        minibosses.Update(1.0f, {}, shots);
        Check(minibosses.BlastEventCount() > 0, "Grave Warden resolves a telegraphed slam or shockwave");
        minibosses.Damage(0, 100000.0f, {}, 0.0f, DamageSource::PlayerArea);
        Check(minibosses.DeathEventCount() == 1 && minibosses.DeathEvents()[0].miniboss,
              "miniboss death is tagged for its dedicated reward path");

        minibosses.Reset(); shots.Reset();
        Check(minibosses.SpawnMiniboss(EnemyType::VoidStalker, {220.0f, 0.0f}),
              "Void Stalker uses the shared enemy pool");
        minibosses.Update(0.1f, {}, shots);
        minibosses.Update(0.7f, {}, shots);
        minibosses.Update(2.8f, {}, shots);
        minibosses.Update(0.7f, {}, shots);
        Check(shots.ActiveCount(ProjectileOwner::Enemy) >= 5,
              "Void Stalker alternates a telegraphed dash with a five-shot volley");
    }

    Player player;
    player.Reset();
    Check(player.stats.level == 1 && Near(player.stats.criticalChance, 0.05f),
          "player base stats");
    player.stats.armor = 3.0f;
    Check(player.TakeDamage(10.0f) && Near(player.stats.currentHP, 93.0f),
          "armor subtracts predictable damage");
    Check(!player.TakeDamage(10.0f) && Near(player.stats.currentHP, 93.0f),
          "contact invulnerability blocks repeated frame damage");
    player.stats.moveSpeed = std::numeric_limits<float>::infinity();
    player.stats.areaMultiplier = -5.0f;
    player.stats.criticalChance = 4.0f;
    player.stats.currentHP = std::numeric_limits<float>::quiet_NaN();
    player.SanitizeStats();
    Check(Near(player.stats.moveSpeed, 235.0f) && Near(player.stats.areaMultiplier, 0.25f) &&
              Near(player.stats.criticalChance, 0.95f) && Near(player.stats.currentHP, player.stats.maxHP),
          "player stat safety clamps invalid, infinite and NaN runtime values");
    player.Reset();
    player.ToggleDebugInvulnerability();
    Check(!player.TakeDamage(1000.0f), "debug invulnerability");

    {
        const std::filesystem::path settingsPath =
            std::filesystem::temp_directory_path() / "vampire_crys_settings_test.json";
        GameSettings saved;
        saved.masterVolume = 0.37f;
        saved.screenShake = 0.22f;
        saved.fullscreen = true;
        saved.language = GameLanguage::PortugueseBrazil;
        GameSettings loaded;
        const bool roundTrip = saved.Save(settingsPath.string()) && loaded.Load(settingsPath.string());
        Check(roundTrip && Near(loaded.masterVolume, 0.37f) && Near(loaded.screenShake, 0.22f) &&
                  loaded.fullscreen && loaded.language == GameLanguage::PortugueseBrazil,
              "settings JSON persists language with safe complete-file replacement");
        Localization::SetLanguage(loaded.language);
        Check(std::string(Localization::Known("NIGHTMARE")) == "PESADELO" &&
                  std::string(Localization::Text("PLAY", "JOGAR")) == "JOGAR",
              "PT-BR localization resolves known content and interface text");
        Localization::SetLanguage(GameLanguage::English);
        {
            std::ofstream corrupt(settingsPath, std::ios::trunc);
            corrupt << "{ invalid json";
        }
        GameSettings fallback;
        Check(!fallback.Load(settingsPath.string()) && Near(fallback.masterVolume, 0.80f),
              "corrupted settings retain safe defaults without crashing");
        std::error_code cleanupError;
        std::filesystem::remove(settingsPath, cleanupError);
    }

    WeaponManager loadout;
    loadout.Reset();
    Check(loadout.WeaponCount() == 1 && loadout.WeaponLevel(WeaponType::ArcBolt) == 1,
          "run starts with Arc Bolt level 1 only");
    for (int raw = 1; raw < WeaponManager::MaxWeaponSlots; ++raw)
        Check(loadout.AddOrUpgradeWeapon(static_cast<WeaponType>(raw)), "add weapon slot");
    Check(!loadout.AddOrUpgradeWeapon(WeaponType::SoulScythe),
          "six weapon slot cap rejects a seventh weapon");
    Check(loadout.WeaponCount() == WeaponManager::MaxWeaponSlots, "six weapon slot cap");
    for (int level = 1; level < WeaponManager::MaxWeaponLevel; ++level)
        loadout.AddOrUpgradeWeapon(WeaponType::ArcBolt);
    Check(loadout.WeaponLevel(WeaponType::ArcBolt) == 8 &&
              !loadout.AddOrUpgradeWeapon(WeaponType::ArcBolt),
          "weapon max level 8");

    loadout.Reset();
    player.Reset();
    for (int raw = 0; raw < WeaponManager::MaxPassiveSlots; ++raw)
        Check(loadout.AddOrUpgradePassive(static_cast<PassiveType>(raw), player.stats),
              "add passive slot");
    Check(loadout.PassiveCount() == 6 &&
              !loadout.AddOrUpgradePassive(PassiveType::IronShell, player.stats),
          "six passive slot cap blocks a new passive");

    loadout.Reset();
    player.Reset();
    for (int level = 0; level < 5; ++level)
        loadout.AddOrUpgradePassive(PassiveType::PowerCore, player.stats);
    Check(Near(player.stats.damageMultiplier, 1.5f), "Power Core affects global damage");
    Check(!loadout.AddOrUpgradePassive(PassiveType::PowerCore, player.stats),
          "passive max level 5");

    loadout.Reset();
    player.Reset();
    loadout.AddOrUpgradePassive(PassiveType::SwiftBoots, player.stats);
    loadout.AddOrUpgradePassive(PassiveType::ChronoGear, player.stats);
    loadout.AddOrUpgradePassive(PassiveType::ExpansionRune, player.stats);
    loadout.AddOrUpgradePassive(PassiveType::MagnetStone, player.stats);
    loadout.AddOrUpgradePassive(PassiveType::VitalHeart, player.stats);
    loadout.AddOrUpgradePassive(PassiveType::LuckyCharm, player.stats);
    Check(player.stats.moveSpeed > 235.0f && player.stats.cooldownMultiplier < 1.0f &&
              player.stats.areaMultiplier > 1.0f && player.stats.magnetRadius > 105.0f &&
              player.stats.maxHP > 100.0f && player.stats.hpRegeneration > 0.0f &&
              player.stats.luck > 0.0f && player.stats.criticalChance > 0.05f,
          "all equipped passive modifiers alter runtime stats");
    Check(RarityWeight(Rarity::Common, 1.0f) < RarityWeight(Rarity::Common, 0.0f) &&
              RarityWeight(Rarity::Epic, 1.0f) > RarityWeight(Rarity::Epic, 0.0f),
          "luck changes rarity weights");

    loadout.Reset();
    player.Reset();
    loadout.AddOrUpgradePassive(PassiveType::IronShell, player.stats);
    Check(Near(player.stats.armor, 1.0f), "Iron Shell changes armor");

    {
        XPOrbManager baseMagnet;
        baseMagnet.Spawn({120.0f, 0.0f}, 1);
        for (int frame = 0; frame < 120; ++frame)
            baseMagnet.Update(1.0f / 60.0f, {0.0f, 0.0f}, 18.0f, 105.0f);
        XPOrbManager improvedMagnet;
        improvedMagnet.Spawn({120.0f, 0.0f}, 1);
        int attractedXP = 0;
        for (int frame = 0; frame < 120; ++frame)
            attractedXP += improvedMagnet.Update(1.0f / 60.0f, {0.0f, 0.0f}, 18.0f, 135.0f);
        Check(baseMagnet.ActiveCount() == 1 && attractedXP == 1,
              "Magnet Stone radius changes XP collection behavior");
    }

    ProjectileManager projectiles;
    Projectile specification{};
    specification.lifetime = 1.0f;
    for (int index = 0; index < 60; ++index) {
        specification.owner = index < 50 ? ProjectileOwner::Player : ProjectileOwner::Enemy;
        Check(projectiles.Spawn(specification), "projectile pool stress spawn");
    }
    Check(projectiles.ActiveCount(ProjectileOwner::Player) == 50 &&
              projectiles.ActiveCount(ProjectileOwner::Enemy) == 10,
          "owner-specific projectile counts");
    projectiles.Reset();
    Check(projectiles.ActiveCount() == 0, "projectile reset");
    Projectile reusedBoss{};
    reusedBoss.owner = ProjectileOwner::Boss;
    reusedBoss.explosive = true;
    reusedBoss.remainingPierces = 99;
    reusedBoss.lifetime = 1.0f;
    projectiles.Spawn(reusedBoss);
    projectiles.ClearOwner(ProjectileOwner::Boss);
    Projectile reusedPlayer{};
    reusedPlayer.owner = ProjectileOwner::Player;
    reusedPlayer.sourceWeapon = WeaponType::ArcBolt;
    reusedPlayer.lifetime = 1.0f;
    projectiles.Spawn(reusedPlayer);
    const Projectile& cleanProjectile = projectiles.Items()[0];
    Check(cleanProjectile.owner == ProjectileOwner::Player && !cleanProjectile.explosive &&
              cleanProjectile.remainingPierces == 0,
          "reused boss projectile slot is fully overwritten for a player shot");
    projectiles.Reset();

    {
        Projectile moving{};
        moving.velocity = {123.0f, -47.0f};
        moving.lifetime = 2.0f;
        ProjectileManager at30;
        ProjectileManager at120;
        at30.Spawn(moving);
        at120.Spawn(moving);
        for (int frame = 0; frame < 30; ++frame) at30.Update(1.0f / 30.0f);
        for (int frame = 0; frame < 120; ++frame) at120.Update(1.0f / 120.0f);
        Check(Near(at30.Items()[0].position.x, at120.Items()[0].position.x, 0.01f) &&
                  Near(at30.Items()[0].position.y, at120.Items()[0].position.y, 0.01f),
              "projectile movement is stable across 30 and 120 FPS timesteps");
    }

    EnemyManager enemies;
    for (int index = 0; index < 160; ++index)
        enemies.Spawn(static_cast<EnemyType>(index % static_cast<int>(EnemyType::Count)),
                      {static_cast<float>(index), 0.0f});
    Check(enemies.ActiveCount() == 160, "150+ enemy pool stress");
    enemies.Reset();
    Check(enemies.ActiveCount() == 0, "enemy reset");

    {
        EnemyManager gridEnemies;
        ProjectileManager gridShots;
        gridEnemies.Spawn(EnemyType::Ghoul, {0.0f, 0.0f});
        gridEnemies.Spawn(EnemyType::Ghoul, {22.0f, 0.0f});
        gridEnemies.Spawn(EnemyType::Ghoul, {600.0f, 0.0f});
        gridEnemies.RebuildGrid();
        std::vector<int> nearby;
        gridEnemies.QueryCircle({0.0f, 0.0f}, 160.0f, nearby);
        const std::set<int> unique(nearby.begin(), nearby.end());
        const Enemy* nearest = gridEnemies.FindNearest({5.0f, 0.0f});
        const bool nearestCorrect = nearest != nullptr && Near(nearest->position.x, 0.0f);
        const float before = std::fabs(gridEnemies.Items()[1].position.x -
                                       gridEnemies.Items()[0].position.x);
        gridEnemies.Update(0.0f, {1000.0f, 0.0f}, gridShots);
        const float after = std::fabs(gridEnemies.Items()[1].position.x -
                                      gridEnemies.Items()[0].position.x);
        Check(nearby.size() == unique.size() && nearby.size() == 2 && nearestCorrect &&
                  after > before,
              "spatial grid query has no duplicates, nearest works and separation repels");
    }

    XPOrbManager orbs;
    for (int index = 0; index < 200; ++index) orbs.Spawn({0.0f, 0.0f}, 1);
    Check(orbs.ActiveCount() == 200, "XP orb pool stress");
    orbs.Reset();
    Check(orbs.ActiveCount() == 0, "XP orb reset");

    AreaEffectManager areas;
    AreaEffect area{};
    area.lifetime = 1.0f;
    Check(areas.Spawn(area) && areas.ActiveCount() == 1, "area pool spawn");
    areas.Reset();
    Check(areas.ActiveCount() == 0, "area reset");

    // Individual weapon behavior checks use a fresh loadout so Arc Bolt's initial
    // cooldown stays above the 0.2s step while the newly equipped weapon fires.
    {
        WeaponManager weapons;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager attackAreas;
        Player owner;
        weapons.Reset(); owner.Reset();
        targets.Spawn(EnemyType::Ghoul, {220.0f, 0.0f});
        targets.RebuildGrid();
        weapons.Update(0.30f, owner, targets, shots, attackAreas);
        Check(shots.ActiveCount(ProjectileOwner::Player) == 1, "Arc Bolt fires");
    }
    {
        WeaponManager weapons;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager manualAreas;
        Player owner;
        weapons.Reset(); owner.Reset(); weapons.SetWeaponSlotLimit(2);
        targets.Spawn(EnemyType::Ghoul, {220.0f, 0.0f}); targets.RebuildGrid();
        AttackRequest idle{{1.0f, 0.0f}, {220.0f, 0.0f}, false, false};
        weapons.UpdateManual(1.0f, idle, owner, targets, shots, manualAreas);
        Check(shots.ActiveCount(ProjectileOwner::Player) == 0,
              "Expedition manual combat does not auto-attack while idle");
        AttackRequest primary{{1.0f, 0.0f}, {220.0f, 0.0f}, true, false};
        weapons.UpdateManual(0.01f, primary, owner, targets, shots, manualAreas);
        Check(shots.ActiveCount(ProjectileOwner::Player) == 1,
              "LMB request fires only the primary Expedition weapon");
        weapons.AddOrUpgradeWeapon(WeaponType::FlameRing);
        const float primaryCooldown = weapons.SlotCooldown(0);
        Check(weapons.SwapWeaponSlots() && Near(weapons.SlotCooldown(1), primaryCooldown, 0.01f) &&
                  weapons.Weapons()[0].type == WeaponType::FlameRing,
              "Q-style slot swap preserves cooldown with each weapon instance");
        AttackRequest secondary{{1.0f, 0.0f}, {220.0f, 0.0f}, false, true};
        weapons.UpdateManual(0.01f, secondary, owner, targets, shots, manualAreas);
        Check(shots.ActiveCount(ProjectileOwner::Player) == 1,
              "RMB request does not trigger the primary slot");
        Check(!weapons.AddOrUpgradeWeapon(WeaponType::SpectralFan) && weapons.WeaponCount() == 2,
              "Expedition loadout enforces exactly two weapon slots");
        Check(weapons.ReplaceWeapon(0, WeaponType::SpectralFan) &&
                  weapons.Weapons()[0].type == WeaponType::SpectralFan &&
                  weapons.Weapons()[0].level == 1,
              "full dual-weapon loadout can replace a selected slot with a level-one weapon");
    }
    {
        WeaponManager baseWeapons;
        WeaponManager poweredWeapons;
        EnemyManager baseTargets;
        EnemyManager poweredTargets;
        ProjectileManager baseShots;
        ProjectileManager poweredShots;
        AreaEffectManager baseAreas;
        AreaEffectManager poweredAreas;
        Player baseOwner;
        Player poweredOwner;
        baseWeapons.Reset(); poweredWeapons.Reset(); baseOwner.Reset(); poweredOwner.Reset();
        baseOwner.stats.criticalChance = 0.0f;
        poweredOwner.stats.criticalChance = 0.0f;
        poweredWeapons.AddOrUpgradePassive(PassiveType::PowerCore, poweredOwner.stats);
        baseTargets.Spawn(EnemyType::Ghoul, {220.0f, 0.0f});
        poweredTargets.Spawn(EnemyType::Ghoul, {220.0f, 0.0f});
        baseTargets.RebuildGrid(); poweredTargets.RebuildGrid();
        baseWeapons.Update(0.30f, baseOwner, baseTargets, baseShots, baseAreas);
        poweredWeapons.Update(0.30f, poweredOwner, poweredTargets, poweredShots, poweredAreas);
        float baseDamage = 0.0f;
        float poweredDamage = 0.0f;
        for (const Projectile& shot : baseShots.Items()) if (shot.active) { baseDamage = shot.damage; break; }
        for (const Projectile& shot : poweredShots.Items()) if (shot.active) { poweredDamage = shot.damage; break; }
        Check(poweredDamage > baseDamage, "Power Core increases actual weapon damage");
    }
    {
        WeaponManager weapons;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager attackAreas;
        Player owner;
        weapons.Reset(); owner.Reset();
        weapons.AddOrUpgradeWeapon(WeaponType::FlameRing);
        targets.Spawn(EnemyType::Ghoul, {50.0f, 0.0f});
        targets.RebuildGrid();
        const float hpBefore = targets.Items()[0].hp;
        weapons.Update(0.20f, owner, targets, shots, attackAreas);
        attackAreas.Update(0.01f, owner.Position(), targets);
        Check(attackAreas.ActiveCount() == 1 && targets.Items()[0].hp < hpBefore,
              "Flame Ring creates a ticking damaging area");
    }
    {
        WeaponManager weapons;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager attackAreas;
        Player owner;
        weapons.Reset(); owner.Reset();
        weapons.AddOrUpgradeWeapon(WeaponType::FlameRing);
        weapons.AddOrUpgradePassive(PassiveType::ExpansionRune, owner.stats);
        weapons.Update(0.20f, owner, targets, shots, attackAreas);
        float radius = 0.0f;
        for (const AreaEffect& effect : attackAreas.Items()) if (effect.active) { radius = effect.radius; break; }
        Check(radius > 105.0f, "Expansion Rune increases actual AoE radius");
    }
    {
        WeaponManager weapons;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager attackAreas;
        Player owner;
        weapons.Reset(); owner.Reset();
        weapons.AddOrUpgradeWeapon(WeaponType::GuardianOrbs);
        targets.Spawn(EnemyType::Ghoul, {94.0f, 2.0f});
        targets.RebuildGrid();
        const float hpBefore = targets.Items()[0].hp;
        weapons.Update(0.01f, owner, targets, shots, attackAreas);
        Check(targets.Items()[0].hp < hpBefore && targets.Items()[0].orbitalHitCooldown > 0.0f,
              "Guardian Orbs collide with per-target cooldown");
    }
    {
        WeaponManager weapons;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager attackAreas;
        Player owner;
        weapons.Reset(); owner.Reset();
        weapons.AddOrUpgradeWeapon(WeaponType::SpectralFan);
        targets.Spawn(EnemyType::Ghoul, {220.0f, 0.0f});
        targets.RebuildGrid();
        weapons.Update(0.20f, owner, targets, shots, attackAreas);
        float firstY = 0.0f;
        float lastY = 0.0f;
        int seen = 0;
        for (const Projectile& shot : shots.Items()) if (shot.active) {
            if (seen == 0) firstY = shot.velocity.y;
            lastY = shot.velocity.y;
            ++seen;
        }
        Check(seen == 3 && firstY * lastY < 0.0f, "Spectral Fan fires a real spread");
    }
    {
        WeaponManager weapons;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager attackAreas;
        Player owner;
        weapons.Reset(); owner.Reset();
        weapons.AddOrUpgradeWeapon(WeaponType::VoidLance);
        targets.Spawn(EnemyType::Ghoul, {220.0f, 0.0f});
        targets.RebuildGrid();
        weapons.Update(0.20f, owner, targets, shots, attackAreas);
        const Projectile* lance = nullptr;
        for (const Projectile& shot : shots.Items()) if (shot.active) { lance = &shot; break; }
        Check(lance != nullptr && lance->remainingPierces == 4 &&
                  std::fabs(lance->velocity.x) >= 759.0f,
              "Void Lance is fast and highly piercing");
    }
    {
        WeaponManager weapons;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager attackAreas;
        Player owner;
        weapons.Reset(); owner.Reset();
        weapons.AddOrUpgradeWeapon(WeaponType::ThunderCannon);
        targets.Spawn(EnemyType::Ghoul, {220.0f, 0.0f});
        targets.RebuildGrid();
        weapons.Update(0.20f, owner, targets, shots, attackAreas);
        Projectile* cannon = nullptr;
        for (Projectile& shot : shots.Items()) if (shot.active) { cannon = &shot; break; }
        Check(cannon != nullptr && cannon->explosive && cannon->explosionRadius >= 82.0f,
              "Thunder Cannon launches an explosive projectile");
        if (cannon != nullptr) shots.Deactivate(*cannon, true);
        Check(shots.ExplosionEventCount() == 1, "Thunder Cannon queues a real AoE explosion");
    }

    {
        EnemyManager rangedEnemies;
        ProjectileManager enemyShots;
        rangedEnemies.Spawn(EnemyType::Cultist, {275.0f, 0.0f});
        rangedEnemies.Update(1.1f, {0.0f, 0.0f}, enemyShots);
        Check(enemyShots.ActiveCount(ProjectileOwner::Enemy) > 0,
              "Cultist maintains range and fires enemy projectiles");
    }
    {
        EnemyManager bombers;
        ProjectileManager enemyShots;
        bombers.Spawn(EnemyType::Bomber, {100.0f, 0.0f});
        bombers.Update(0.1f, {0.0f, 0.0f}, enemyShots);
        Check(bombers.Items()[0].telegraphing, "Bomber enters telegraph state");
        bombers.Update(1.0f, {0.0f, 0.0f}, enemyShots);
        Check(bombers.BlastEventCount() > 0 && !bombers.Items()[0].active,
              "Bomber explodes and removes itself");
    }

    {
        GameSession session;
        session.Reset();
        session.GrantDebugXP(1000);
        int resolvedLevels = 0;
        while (session.HasPendingLevelUp() && resolvedLevels < 100) {
            session.PrepareUpgradeChoices();
            if (session.UpgradeChoiceCount() <= 0) break;
            session.SelectUpgrade(0);
            ++resolvedLevels;
        }
        Check(resolvedLevels >= 10 && !session.HasPendingLevelUp(),
              "large XP awards resolve every queued level-up one choice at a time");
        session.Reset();
        session.GrantDebugXP(1000);
        bool sawNewWeapon = false;
        bool noDuplicates = true;
        for (int attempt = 0; attempt < 80; ++attempt) {
            session.PrepareUpgradeChoices();
            const auto& choices = session.UpgradeChoices();
            for (int first = 0; first < session.UpgradeChoiceCount(); ++first) {
                const UpgradeChoice& choice = choices[static_cast<std::size_t>(first)];
                sawNewWeapon |= choice.kind == UpgradeKind::Weapon && choice.currentLevel == 0;
                for (int second = first + 1; second < session.UpgradeChoiceCount(); ++second) {
                    const UpgradeChoice& other = choices[static_cast<std::size_t>(second)];
                    const bool same = choice.kind == other.kind &&
                        (choice.kind == UpgradeKind::Weapon ? choice.weapon == other.weapon
                                                            : choice.passive == other.passive);
                    noDuplicates &= !same;
                }
            }
        }
        Check(sawNewWeapon && noDuplicates,
              "available weapon slots offer new weapons without duplicate cards");
    }
    {
        GameSession session;
        session.Reset();
        for (int raw = 1; raw < static_cast<int>(WeaponType::Count); ++raw)
            session.Loadout().AddOrUpgradeWeapon(static_cast<WeaponType>(raw));
        for (int level = 1; level < WeaponManager::MaxWeaponLevel; ++level)
            session.Loadout().AddOrUpgradeWeapon(WeaponType::ArcBolt);
        session.GrantDebugXP(1000);
        bool offeredNewWeapon = false;
        bool offeredMaxArc = false;
        for (int attempt = 0; attempt < 50; ++attempt) {
            session.PrepareUpgradeChoices();
            for (int index = 0; index < session.UpgradeChoiceCount(); ++index) {
                const UpgradeChoice& choice = session.UpgradeChoices()[static_cast<std::size_t>(index)];
                offeredNewWeapon |= choice.kind == UpgradeKind::Weapon && choice.currentLevel == 0;
                offeredMaxArc |= choice.kind == UpgradeKind::Weapon &&
                                 choice.weapon == WeaponType::ArcBolt;
            }
        }
        Check(!offeredNewWeapon && !offeredMaxArc,
              "full weapon slots and level 8 weapons are excluded");
    }
    {
        GameSession session;
        session.Reset();
        for (int raw = 0; raw < WeaponManager::MaxPassiveSlots; ++raw)
            session.Loadout().AddOrUpgradePassive(static_cast<PassiveType>(raw),
                                                  session.GetPlayer().stats);
        for (int level = 1; level < WeaponManager::MaxPassiveLevel; ++level)
            session.Loadout().AddOrUpgradePassive(PassiveType::SwiftBoots,
                                                  session.GetPlayer().stats);
        session.GrantDebugXP(1000);
        bool offeredNewPassive = false;
        bool offeredMaxBoots = false;
        for (int attempt = 0; attempt < 50; ++attempt) {
            session.PrepareUpgradeChoices();
            for (int index = 0; index < session.UpgradeChoiceCount(); ++index) {
                const UpgradeChoice& choice = session.UpgradeChoices()[static_cast<std::size_t>(index)];
                offeredNewPassive |= choice.kind == UpgradeKind::Passive && choice.currentLevel == 0;
                offeredMaxBoots |= choice.kind == UpgradeKind::Passive &&
                                   choice.passive == PassiveType::SwiftBoots;
            }
        }
        Check(!offeredNewPassive && !offeredMaxBoots,
              "full passive slots and level 5 passives are excluded");
    }

    {
        GameSession soak;
        auto modes = std::make_unique<GameModeManager>();
        modes->ActivateMode(GameModeType::Survival, soak);
        soak.GetPlayer().ToggleDebugInvulnerability();
        for (int raw = 1; raw < static_cast<int>(WeaponType::Count); ++raw)
            soak.Loadout().AddOrUpgradeWeapon(static_cast<WeaponType>(raw));
        for (int raw = 0; raw < WeaponManager::MaxPassiveSlots; ++raw)
            soak.Loadout().AddOrUpgradePassive(static_cast<PassiveType>(raw),
                                               soak.GetPlayer().stats);
        for (int index = 0; index < 200; ++index) {
            const float angle = static_cast<float>(index) * 0.31f;
            const float radius = 180.0f + static_cast<float>(index % 12) * 38.0f;
            soak.Enemies().Spawn(static_cast<EnemyType>(index % static_cast<int>(EnemyType::Count)),
                                 {std::cos(angle) * radius, std::sin(angle) * radius});
        }
        for (int frame = 0; frame < 1800; ++frame) {
            modes->UpdateActiveMode(1.0f / 30.0f, soak);
            if (soak.HasPendingLevelUp()) {
                soak.PrepareUpgradeChoices();
                if (soak.UpgradeChoiceCount() > 0) soak.SelectUpgrade(0);
            }
        }
        Check(!soak.IsGameOver() && soak.ElapsedTime() >= 59.9f && soak.Kills() > 0 &&
                  soak.Projectiles().ActiveCount() >= 0 && soak.Enemies().ActiveCount() >= 0,
              "60-second full-loadout soak with 200 mixed enemies");
    }

    {
        WaveDirector director;
        EnemyManager waveEnemies;
        Check(director.LoadedExternalConfig(), "waves.json loads successfully");
        const float checkpoints[]{0.0f, 180.0f, 300.0f, 480.0f, 600.0f, 780.0f, 870.0f};
        const int expectedPhases[]{0, 3, 4, 5, 6, 7, 8};
        bool phasesMatch = true;
        for (int index = 0; index < 7; ++index) {
            director.Update(0.0f, checkpoints[index], {0.0f, 0.0f}, 1.0f, waveEnemies);
            phasesMatch &= director.CurrentWaveIndex() == expectedPhases[index];
        }
        director.Update(0.0f, 900.0f, {0.0f, 0.0f}, 1.0f, waveEnemies);
        const DifficultyState late = director.CurrentDifficulty();
        Check(phasesMatch && Near(late.hp, 2.0f) && Near(late.damage, 1.45f) &&
                  late.speed <= 1.151f && Near(late.eliteChance, 0.08f),
              "wave phases and controlled 15-minute scaling");
    }

    {
        EnemyManager eliteEnemies;
        eliteEnemies.Spawn(EnemyType::Ghoul, {0.0f, 0.0f}, 1, 1, 1, 1, false);
        eliteEnemies.Spawn(EnemyType::Ghoul, {100.0f, 0.0f}, 1, 1, 1, 1, true);
        const Enemy normal = eliteEnemies.Items()[0];
        const Enemy elite = eliteEnemies.Items()[1];
        Check(elite.elite && elite.maxHP >= normal.maxHP * 3.0f &&
                  elite.contactDamage >= normal.contactDamage * 1.5f &&
                  elite.xpReward >= normal.xpReward * 3 && elite.radius > normal.radius,
              "elite stat, XP and size modifiers");
        eliteEnemies.Reset();
        eliteEnemies.Spawn(EnemyType::Ghoul, {0.0f, 0.0f});
        Check(!eliteEnemies.Items()[0].elite, "elite flag does not leak through pool reuse");
    }

    {
        PickupManager pickupPool;
        pickupPool.Spawn({0.0f, 0.0f}, PickupType::Health);
        pickupPool.Spawn({0.0f, 0.0f}, PickupType::Magnet);
        pickupPool.Spawn({0.0f, 0.0f}, PickupType::Bomb);
        const PickupResult result = pickupPool.Update(0.01f, {0.0f, 0.0f}, 18.0f);
        Check(result.health == 1 && result.magnet == 1 && result.bomb == 1 &&
                  pickupPool.ActiveCount() == 0,
              "health, magnet and bomb pickups collect and return to pool");
        LootSystem loot;
        Check(loot.ChanceMultiplier(1.0f) > loot.ChanceMultiplier(0.0f) &&
                  loot.ChanceMultiplier(100.0f) <= 2.0f,
              "Luck improves drop chance with a safe clamp");
    }

    {
        GameSession pickupSession;
        pickupSession.Reset();
        pickupSession.GetPlayer().TakeDamage(50.0f);
        pickupSession.Pickups().Spawn(pickupSession.GetPlayer().Position(), PickupType::Health);
        pickupSession.Update(0.01f);
        Check(pickupSession.GetPlayer().stats.currentHP > 50.0f,
              "health pickup restores runtime player HP");
        pickupSession.XPOrbs().Spawn({350.0f, 0.0f}, 9);
        pickupSession.Pickups().Spawn(pickupSession.GetPlayer().Position(), PickupType::Magnet);
        for (int frame = 0; frame < 180; ++frame) pickupSession.Update(1.0f / 60.0f);
        Check(pickupSession.Statistics().xpCollected >= 9,
              "magnet pickup attracts all active XP orbs without teleporting");
        pickupSession.Enemies().Spawn(EnemyType::Ghoul, {80.0f, 0.0f});
        pickupSession.Enemies().RebuildGrid();
        const int killsBeforeBomb = pickupSession.Kills();
        pickupSession.Pickups().Spawn(pickupSession.GetPlayer().Position(), PickupType::Bomb);
        pickupSession.Update(0.01f);
        Check(pickupSession.Kills() > killsBeforeBomb,
              "bomb pickup damages visible normal enemies through the grid");
    }

    {
        GameSession victory;
        auto modes = std::make_unique<GameModeManager>();
        modes->ActivateMode(GameModeType::Survival, victory);
        modes->DebugSkipTime(900.0f, victory);
        modes->UpdateActiveMode(0.01f, victory);
        Check(modes->Context().outcome == RunOutcome::None,
              "15 minutes no longer grants automatic Victory");
        victory.DebugDamageBoss(1.1f);
        modes->UpdateActiveMode(0.0f, victory);
        Check(modes->Context().outcome == RunOutcome::None && victory.Chests().ActiveCount() == 1,
              "intermediate boss death is not Victory and always drops a chest");
        victory.DebugSpawnBoss(BossType::VoidHeraldAscended);
        victory.DebugDamageBoss(1.1f);
        modes->UpdateActiveMode(0.0f, victory);
        Check(modes->Context().outcome == RunOutcome::Victory && victory.Chests().ActiveCount() == 1 &&
                  victory.Statistics().bossesKilled == 2,
              "final boss death grants Victory without dropping another chest");
        Check(modes->Result().mode == GameModeType::Survival && modes->Result().victory &&
                  modes->Result().outcome == RunOutcome::Victory &&
                  modes->Result().bossesKilled == 2,
              "completed run produces a shared Survival RunResult snapshot");
        Projectile hostile{};
        hostile.lifetime = 10.0f;
        hostile.owner = ProjectileOwner::Boss;
        victory.Projectiles().Spawn(hostile);
        victory.ClearBossHazards();
        Check(victory.Projectiles().ActiveCount(ProjectileOwner::Boss) == 0 &&
                  victory.Bosses().TelegraphCount() == 0,
              "Game Over/Victory cleanup removes boss projectiles and hazards");
        modes->RestartActiveMode(victory);
        Check(modes->Context().outcome == RunOutcome::None && Near(victory.ElapsedTime(), 0.0f) &&
                  victory.Enemies().ActiveCount() == 0 && victory.Pickups().ActiveCount() == 0 &&
                  victory.Particles().ActiveCount() == 0 && !victory.Bosses().IsActive() &&
                  victory.Chests().ActiveCount() == 0 && victory.Loadout().EvolutionCount() == 0 &&
                  victory.Statistics().bossesKilled == 0,
              "restart resets bosses, queue, hazards, chests, evolutions and run statistics");
    }

    {
        GameSession retry;
        retry.Reset();
        retry.GetPlayer().TakeDamage(10000.0f);
        Check(retry.IsGameOver(), "lethal damage deterministically reaches Game Over");
        retry.Reset();
        Check(!retry.IsGameOver() && retry.GetPlayer().stats.level == 1 &&
                  Near(retry.GetPlayer().stats.currentHP, retry.GetPlayer().stats.maxHP) &&
                  retry.Loadout().WeaponCount() == 1 && retry.Kills() == 0,
              "Game Over retry starts a clean base run");
    }

    {
        GameSession interruptedBoss;
        auto modes = std::make_unique<GameModeManager>();
        modes->ActivateMode(GameModeType::Survival, interruptedBoss);
        interruptedBoss.GetPlayer().ToggleDebugInvulnerability();
        interruptedBoss.DebugSpawnBoss(BossType::VoidHerald);
        bool reachedTelegraph = false;
        for (int frame = 0; frame < 400 && !reachedTelegraph; ++frame) {
            modes->UpdateActiveMode(1.0f / 60.0f, interruptedBoss);
            reachedTelegraph = interruptedBoss.Bosses().TelegraphCount() > 0;
        }
        interruptedBoss.DebugDamageBoss(1.1f);
        modes->UpdateActiveMode(0.0f, interruptedBoss);
        Check(reachedTelegraph && interruptedBoss.Bosses().TelegraphCount() == 0 &&
                  interruptedBoss.Projectiles().ActiveCount(ProjectileOwner::Boss) == 0 &&
                  interruptedBoss.Chests().ActiveCount() == 1,
              "boss death during telegraph clears hazards and still creates its reward");
    }

    {
        Telegraph circle{};
        circle.type = TelegraphType::Circle;
        circle.position = {0.0f, 0.0f};
        circle.radius = 50.0f;
        Telegraph cone{};
        cone.type = TelegraphType::Cone;
        cone.position = {0.0f, 0.0f};
        cone.direction = {1.0f, 0.0f};
        cone.range = 300.0f;
        cone.angleDegrees = 60.0f;
        Telegraph line{};
        line.type = TelegraphType::Line;
        line.position = {0.0f, 0.0f};
        line.direction = {1.0f, 0.0f};
        line.range = 400.0f;
        line.width = 40.0f;
        Check(TelegraphManager::CircleHits(circle, {55.0f, 0.0f}, 6.0f) &&
                  TelegraphManager::ConeHits(cone, {200.0f, 20.0f}, 10.0f) &&
                  !TelegraphManager::ConeHits(cone, {-100.0f, 0.0f}, 10.0f) &&
                  TelegraphManager::LineHits(line, {200.0f, 24.0f}, 5.0f) &&
                  !TelegraphManager::LineHits(line, {200.0f, 40.0f}, 5.0f),
              "circle, cone and thick-line telegraph geometry matches damage zones");
    }

    {
        Player bossTarget;
        bossTarget.Reset();
        bossTarget.ToggleDebugInvulnerability();
        ProjectileManager bossShots;
        BossManager wyrm;
        wyrm.Reset();
        wyrm.SpawnDebug(BossType::FlameWyrm, bossTarget.Position());
        bool sawBreath = false;
        bool sawPools = false;
        bool sawTelegraph = false;
        for (int frame = 0; frame < 700; ++frame) {
            wyrm.Update(1.0f / 30.0f, bossTarget, bossShots);
            bossShots.Update(1.0f / 30.0f);
            sawBreath |= wyrm.ActiveBoss().currentAttack == BossAttack::FireBreath;
            sawPools |= wyrm.ActiveBoss().currentAttack == BossAttack::FlamePools;
            sawTelegraph |= wyrm.TelegraphCount() > 0;
        }
        wyrm.Damage(wyrm.ActiveBoss().maxHP * 0.55f, DamageSource::PlayerArea);
        wyrm.Update(0.01f, bossTarget, bossShots);
        Check(sawBreath && sawPools && sawTelegraph && wyrm.ActiveBoss().phase == 2,
              "Flame Wyrm cycles Fire Breath and Flame Pools with telegraphs and phase 2");
        wyrm.Damage(wyrm.ActiveBoss().currentHP + 1.0f, DamageSource::PlayerProjectile);
        Check(wyrm.HasDeathEvent() && !wyrm.ConsumeDeathEvent().finalBoss,
              "Flame Wyrm death emits an intermediate reward event");
    }

    {
        Player bossTarget;
        bossTarget.Reset();
        bossTarget.ToggleDebugInvulnerability();
        ProjectileManager bossShots;
        BossManager herald;
        herald.Reset();
        herald.SpawnDebug(BossType::VoidHerald, bossTarget.Position());
        bool sawBarrage = false;
        bool sawBeams = false;
        int peakBossShots = 0;
        for (int frame = 0; frame < 700; ++frame) {
            herald.Update(1.0f / 30.0f, bossTarget, bossShots);
            bossShots.Update(1.0f / 30.0f);
            sawBarrage |= herald.ActiveBoss().currentAttack == BossAttack::RadialBarrage;
            sawBeams |= herald.ActiveBoss().currentAttack == BossAttack::VoidBeams;
            peakBossShots = std::max(peakBossShots, bossShots.ActiveCount(ProjectileOwner::Boss));
        }
        herald.Damage(herald.ActiveBoss().maxHP * 0.55f, DamageSource::PlayerArea);
        herald.Update(0.01f, bossTarget, bossShots);
        Check(sawBarrage && sawBeams && peakBossShots >= 12 && herald.ActiveBoss().phase == 2,
              "Void Herald cycles pooled radial projectiles and fair beams with phase 2");
    }

    {
        GameSession queue;
        auto modes = std::make_unique<GameModeManager>();
        modes->ActivateMode(GameModeType::Survival, queue);
        queue.GetPlayer().ToggleDebugInvulnerability();
        modes->DebugSkipTime(900.0f, queue);
        modes->UpdateActiveMode(0.01f, queue);
        const bool flameFirst = queue.Bosses().IsActive() &&
                                queue.Bosses().ActiveBoss().type == BossType::FlameWyrm;
        const bool noOverlap = modes->HUDData().pendingEncounters == 3;
        queue.DebugDamageBoss(1.1f);
        modes->UpdateActiveMode(0.0f, queue);
        modes->UpdateActiveMode(1.6f, queue);
        Check(flameFirst && noOverlap && queue.Bosses().IsActive() &&
                  queue.Bosses().ActiveBoss().type == BossType::VoidHerald,
              "boss milestones queue in order without overlapping encounters");
        queue.DebugDamageBoss(1.1f);
        modes->UpdateActiveMode(0.0f, queue);
        modes->UpdateActiveMode(1.6f, queue);
        const bool finalSpawned = queue.Bosses().IsActive() &&
                                  queue.Bosses().ActiveBoss().type == BossType::VoidHeraldAscended;
        queue.DebugDamageBoss(1.1f);
        modes->UpdateActiveMode(0.0f, queue);
        const bool finalVictory = modes->Context().outcome == RunOutcome::Victory;
        Check(finalSpawned && finalVictory && queue.Bosses().TelegraphCount() == 0,
              "accelerated 5/10/15 minute boss sequence ends only after final boss death");
    }

    {
        const std::array<PassiveType, static_cast<std::size_t>(WeaponType::Count)> required{{
            PassiveType::FocusCrystal, PassiveType::EmberCore, PassiveType::OrbitalEngine,
            PassiveType::WindSigil, PassiveType::PiercingEye, PassiveType::TitanCore,
            PassiveType::ExecutionerSigil, PassiveType::FrozenHeart, PassiveType::BloodPact,
            PassiveType::GraviticCore}};
        bool allEvolve = true;
        for (int raw = 0; raw < static_cast<int>(WeaponType::Count); ++raw) {
            WeaponManager evolutionLoadout;
            Player owner;
            evolutionLoadout.Reset(); owner.Reset();
            const WeaponType type = static_cast<WeaponType>(raw);
            while (evolutionLoadout.WeaponLevel(type) < WeaponManager::MaxWeaponLevel)
                if (!evolutionLoadout.AddOrUpgradeWeapon(type)) break;
            evolutionLoadout.AddOrUpgradePassive(required[static_cast<std::size_t>(raw)], owner.stats);
            allEvolve &= evolutionLoadout.IsEvolutionEligible(type) &&
                         evolutionLoadout.EvolveWeapon(type) && evolutionLoadout.IsEvolved(type) &&
                         evolutionLoadout.WeaponCount() <= WeaponManager::MaxWeaponSlots;
        }
        Check(allEvolve, "all ten evolution recipes require level 8 plus their owned passive");
    }

    {
        GameSession chest;
        chest.Reset();
        chest.DebugPrepareEvolution(WeaponType::ArcBolt);
        chest.Update(0.01f);
        Check(chest.HasPendingChestReward() && chest.ChestChoiceCount() == 1 &&
                  chest.ChestChoices()[0].kind == ChestRewardKind::Evolution,
              "boss chest prioritizes a single eligible evolution");
        chest.SelectChestReward(0);
        Check(chest.Loadout().Evolution(WeaponType::ArcBolt) == EvolvedWeaponType::StormArc &&
                  chest.Loadout().WeaponCount() == 1 && chest.Statistics().chestsOpened == 1,
              "Storm Arc replaces Arc Bolt in the same slot");
        chest.GrantDebugXP(1000);
        chest.PrepareUpgradeChoices();
        bool offeredArc = false;
        for (int index = 0; index < chest.UpgradeChoiceCount(); ++index)
            offeredArc |= chest.UpgradeChoices()[static_cast<std::size_t>(index)].kind == UpgradeKind::Weapon &&
                          chest.UpgradeChoices()[static_cast<std::size_t>(index)].weapon == WeaponType::ArcBolt;
        Check(!offeredArc, "evolved base weapon no longer appears in normal level-up choices");
        chest.Reset();
        Check(chest.Loadout().WeaponLevel(WeaponType::ArcBolt) == 1 &&
                  !chest.Loadout().IsEvolved(WeaponType::ArcBolt),
              "evolution does not persist through restart");
    }

    {
        GameSession choices;
        choices.Reset();
        choices.DebugPrepareEvolution(WeaponType::ArcBolt);
        choices.DebugPrepareEvolution(WeaponType::FlameRing);
        choices.DebugPrepareEvolution(WeaponType::GuardianOrbs);
        choices.Update(0.01f);
        Check(choices.ChestChoiceCount() == 3 &&
                  choices.ChestChoices()[0].kind == ChestRewardKind::Evolution &&
                  choices.ChestChoices()[1].kind == ChestRewardKind::Evolution &&
                  choices.ChestChoices()[2].kind == ChestRewardKind::Evolution,
              "chest presents three eligible evolutions without choosing randomly");
        choices.SelectChestReward(0);
        Check(choices.Loadout().EvolutionCount() == 1 &&
                  choices.Loadout().IsEvolutionEligible(WeaponType::FlameRing) &&
                  choices.Loadout().IsEvolutionEligible(WeaponType::GuardianOrbs),
              "choosing one evolution leaves the other eligible recipes intact");
        GameSession fallback;
        fallback.Reset();
        fallback.DebugSpawnChest();
        fallback.Update(0.01f);
        Check(fallback.HasPendingChestReward() && fallback.ChestChoiceCount() > 0 &&
                  fallback.ChestChoices()[0].kind != ChestRewardKind::Evolution,
              "chest without evolution always offers a valid useful fallback");

        GameSession maxed;
        maxed.Reset();
        for (int raw = 0; raw < static_cast<int>(WeaponType::Count); ++raw)
            while (maxed.Loadout().WeaponLevel(static_cast<WeaponType>(raw)) < 8)
                if (!maxed.Loadout().AddOrUpgradeWeapon(static_cast<WeaponType>(raw))) break;
        for (int raw = 0; raw < WeaponManager::MaxPassiveSlots; ++raw)
            for (int level = 0; level < WeaponManager::MaxPassiveLevel; ++level)
                maxed.Loadout().AddOrUpgradePassive(static_cast<PassiveType>(raw), maxed.GetPlayer().stats);
        maxed.DebugSpawnChest();
        maxed.Update(0.01f);
        Check(maxed.ChestChoiceCount() == 1 &&
                  maxed.ChestChoices()[0].kind == ChestRewardKind::Recovery,
              "fully maxed build receives recovery and XP instead of an empty chest");
    }

    {
        EnemyManager statusTargets;
        ProjectileManager statusShots;
        statusTargets.Spawn(EnemyType::Ghoul, {100.0f, 0.0f});
        statusTargets.ApplySlow(0, 0.10f, 0.05f);
        Check(Near(statusTargets.Items()[0].slowMultiplier, 0.55f),
              "Slow has a safe lower bound and cannot reduce normal enemy speed to zero");
        statusTargets.Update(0.11f, {}, statusShots);
        Check(Near(statusTargets.Items()[0].slowMultiplier, 1.0f),
              "Slow expires and restores normal movement multiplier");

        statusTargets.Reset();
        statusTargets.SpawnMiniboss(EnemyType::GraveWarden, {100.0f, 0.0f});
        statusTargets.ApplySlow(0, 2.0f, 0.55f);
        Check(statusTargets.Items()[0].slowMultiplier > 0.79f &&
                  statusTargets.Items()[0].slowRemaining < 1.2f,
              "minibosses receive reduced Slow strength and duration");

        BossManager resistantBoss;
        resistantBoss.SpawnDebug(BossType::FlameWyrm, {});
        resistantBoss.ApplySlow(3.0f, 0.10f);
        Check(Near(resistantBoss.ActiveBoss().slowMultiplier, 0.88f) &&
                  resistantBoss.ActiveBoss().slowRemaining <= 1.21f,
              "boss Slow is strongly capped and duration-reduced");
        const Vector2 bossVelocityBefore = resistantBoss.ActiveBoss().position;
        statusTargets.ApplyPull(0, {}, 100.0f);
        Check(statusTargets.Items()[0].knockbackVelocity.x < 0.0f &&
                  Near(resistantBoss.ActiveBoss().position.x, bossVelocityBefore.x),
              "gravity pull affects minibosses weakly while boss state has no pull path");
    }

    {
        Player owner;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager stage9Areas;
        owner.Reset();
        owner.stats.criticalChance = 0.0f;

        WeaponManager scythe;
        scythe.Reset(WeaponType::SoulScythe);
        targets.Spawn(EnemyType::Brute, {0.0f, 90.0f});
        targets.Spawn(EnemyType::Brute, {20.0f, 105.0f});
        targets.RebuildGrid();
        const float firstHP = targets.Items()[0].hp;
        scythe.Update(0.30f, owner, targets, shots, stage9Areas);
        Check(targets.Items()[0].hp < firstHP && shots.ActiveCount() == 0,
              "Soul Scythe performs a direct pooled-grid arc sweep without projectile spam");

        targets.Reset(); shots.Reset(); stage9Areas.Reset();
        targets.Spawn(EnemyType::Brute, {180.0f, 0.0f}); targets.RebuildGrid();
        WeaponManager frost;
        frost.Reset(WeaponType::FrostShards);
        frost.Update(0.30f, owner, targets, shots, stage9Areas);
        bool sawFrost = false;
        for (const Projectile& projectile : shots.Items()) if (projectile.active) {
            sawFrost |= projectile.sourceWeapon == WeaponType::FrostShards &&
                        projectile.slowDuration > 1.0f && projectile.slowMultiplier < 0.8f;
        }
        Check(sawFrost && shots.ActiveCount(ProjectileOwner::Player) >= 3,
              "Frost Shards fires a bounded multi-shard volley carrying Slow metadata");

        shots.Reset();
        targets.Spawn(EnemyType::Brute, {210.0f, 80.0f}); targets.RebuildGrid();
        WeaponManager needles;
        needles.Reset(WeaponType::BloodNeedles);
        while (needles.WeaponLevel(WeaponType::BloodNeedles) < 8)
            needles.AddOrUpgradeWeapon(WeaponType::BloodNeedles);
        needles.Update(0.30f, owner, targets, shots, stage9Areas);
        std::set<int> needleTargets;
        for (const Projectile& projectile : shots.Items()) if (projectile.active &&
                projectile.sourceWeapon == WeaponType::BloodNeedles) needleTargets.insert(projectile.homingTarget);
        Check(shots.ActiveCount(ProjectileOwner::Player) <= 14 && needleTargets.size() >= 2,
              "Blood Needles caps its volley and distributes homing targets across the crowd");

        shots.Reset(); stage9Areas.Reset();
        WeaponManager gravity;
        gravity.Reset(WeaponType::GravityWell);
        gravity.Update(0.30f, owner, targets, shots, stage9Areas);
        Check(stage9Areas.ActiveCount() == 1 && stage9Areas.Items()[0].sourceWeapon == WeaponType::GravityWell &&
                  stage9Areas.Items()[0].pullStrength > 0.0f && stage9Areas.Items()[0].ticksRemaining >= 8,
              "Gravity Well creates one tick-based pooled area with bounded spatial pull queries");

        PlayerStats passiveStats{};
        WeaponManager passives;
        passives.Reset();
        passives.AddOrUpgradePassive(PassiveType::ExecutionerSigil, passiveStats);
        passives.AddOrUpgradePassive(PassiveType::FrozenHeart, passiveStats);
        passives.AddOrUpgradePassive(PassiveType::SoulHarvest, passiveStats);
        passives.AddOrUpgradePassive(PassiveType::BloodPact, passiveStats);
        passives.AddOrUpgradePassive(PassiveType::GraviticCore, passiveStats);
        passives.AddOrUpgradePassive(PassiveType::SoulChain, passiveStats);
        passives.AddOrUpgradePassive(PassiveType::SoulChain, passiveStats);
        Check(passiveStats.lowHealthDamageBonus > 0.0f && passiveStats.slowEffectiveness > 1.0f &&
                  passiveStats.xpMultiplier > 1.0f && passiveStats.damageMultiplier > 1.0f &&
                  passiveStats.controlStrength > 1.0f && passiveStats.projectileCountBonus == 1,
              "all six Stage 9 passives apply their distinct capped runtime modifiers");
    }

    {
        const std::array<WeaponType, 4> newWeapons{{WeaponType::SoulScythe, WeaponType::FrostShards,
                                                    WeaponType::BloodNeedles, WeaponType::GravityWell}};
        const std::array<PassiveType, 4> requirements{{PassiveType::ExecutionerSigil, PassiveType::FrozenHeart,
                                                       PassiveType::BloodPact, PassiveType::GraviticCore}};
        for (std::size_t recipe = 0; recipe < newWeapons.size(); ++recipe) {
            Player owner;
            EnemyManager targets;
            ProjectileManager shots;
            AreaEffectManager recipeAreas;
            WeaponManager evolved;
            owner.Reset(); owner.stats.criticalChance = 0.0f;
            evolved.Reset(newWeapons[recipe]);
            while (evolved.WeaponLevel(newWeapons[recipe]) < 8)
                evolved.AddOrUpgradeWeapon(newWeapons[recipe]);
            evolved.AddOrUpgradePassive(requirements[recipe], owner.stats);
            Check(evolved.IsEvolutionEligible(newWeapons[recipe]) &&
                      evolved.EvolveWeapon(newWeapons[recipe]),
                  "new weapon evolution is chest-eligible only after level 8 plus matching passive");
            targets.Spawn(EnemyType::Brute, {0.0f, 95.0f});
            targets.Spawn(EnemyType::Brute, {140.0f, 20.0f});
            targets.RebuildGrid();
            evolved.Update(0.30f, owner, targets, shots, recipeAreas);
            if (newWeapons[recipe] == WeaponType::FrostShards)
                evolved.Update(3.0f, owner, targets, shots, recipeAreas);
            if (newWeapons[recipe] == WeaponType::SoulScythe)
                Check(targets.Items()[0].hp < targets.Items()[0].maxHP,
                      "Reaper's Covenant executes empowered direct sweeps");
            else if (newWeapons[recipe] == WeaponType::FrostShards)
                Check(recipeAreas.ActiveCount() > 0 && recipeAreas.Items()[0].sourceWeapon == WeaponType::FrostShards,
                      "Absolute Zero alternates shard volleys with a frost nova");
            else if (newWeapons[recipe] == WeaponType::BloodNeedles)
                Check(shots.ActiveCount(ProjectileOwner::Player) >= 7 &&
                          shots.ActiveCount(ProjectileOwner::Player) <= 14,
                      "Crimson Swarm remains bounded while increasing its needle volley");
            else
                Check(recipeAreas.ActiveCount() == 1 && recipeAreas.Items()[0].radius > 180.0f &&
                          recipeAreas.Items()[0].pullStrength > 150.0f,
                      "Singularity creates a larger and stronger tick-based gravity field");
        }
    }

    {
        GameSession projectileBoss;
        projectileBoss.Reset();
        projectileBoss.GetPlayer().ToggleDebugInvulnerability();
        for (int raw = 0; raw < static_cast<int>(WeaponType::Count); ++raw) {
            projectileBoss.DebugSpawnBoss(BossType::FlameWyrm);
            projectileBoss.Bosses().ActiveBoss().position = {70.0f, 0.0f};
            const float hpBefore = projectileBoss.Bosses().ActiveBoss().currentHP;
            Projectile hit{};
            hit.position = projectileBoss.Bosses().ActiveBoss().position;
            hit.damage = 10.0f;
            hit.lifetime = 1.0f;
            hit.radius = 8.0f;
            hit.remainingPierces = 10;
            hit.owner = ProjectileOwner::Player;
            hit.sourceWeapon = static_cast<WeaponType>(raw);
            projectileBoss.Projectiles().Spawn(hit);
            projectileBoss.Update(0.0f);
            Check(projectileBoss.Bosses().ActiveBoss().currentHP < hpBefore,
                  "each player projectile weapon type can damage bosses");
        }
    }

    {
        GameSession piercingBoss;
        piercingBoss.Reset();
        piercingBoss.GetPlayer().ToggleDebugInvulnerability();
        piercingBoss.DebugSpawnBoss(BossType::FlameWyrm);
        piercingBoss.Bosses().ActiveBoss().position = {70.0f, 0.0f};
        Projectile piercing{};
        piercing.position = {70.0f, 0.0f};
        piercing.damage = 25.0f;
        piercing.lifetime = 2.0f;
        piercing.radius = 20.0f;
        piercing.remainingPierces = 100;
        piercing.owner = ProjectileOwner::Player;
        piercingBoss.Projectiles().Spawn(piercing);
        const float before = piercingBoss.Bosses().ActiveBoss().currentHP;
        piercingBoss.Update(0.0f);
        const float afterFirst = piercingBoss.Bosses().ActiveBoss().currentHP;
        piercingBoss.Update(0.0f);
        Check(before - afterFirst >= 25.0f && Near(piercingBoss.Bosses().ActiveBoss().currentHP, afterFirst),
              "one piercing projectile cannot repeatedly hit the same boss");

        BossManager areaBoss;
        areaBoss.Reset();
        areaBoss.SpawnDebug(BossType::FlameWyrm, {0.0f, 0.0f});
        areaBoss.ActiveBoss().position = {80.0f, 0.0f};
        EnemyManager noEnemies;
        AreaEffectManager bossArea;
        AreaEffect effect{};
        effect.position = {0.0f, 0.0f};
        effect.radius = 100.0f;
        effect.damage = 20.0f;
        effect.lifetime = 1.0f;
        effect.ticksRemaining = 1;
        bossArea.Spawn(effect);
        const float beforeArea = areaBoss.ActiveBoss().currentHP;
        bossArea.Update(0.01f, {0.0f, 0.0f}, noEnemies, &areaBoss);
        Check(areaBoss.ActiveBoss().currentHP < beforeArea,
              "player area attacks damage bosses through the boss query path");

        WeaponManager orbitals;
        Player owner;
        ProjectileManager noShots;
        AreaEffectManager noAreas;
        owner.Reset();
        orbitals.Reset();
        orbitals.AddOrUpgradeWeapon(WeaponType::GuardianOrbs);
        areaBoss.ActiveBoss().position = {94.0f, 0.0f};
        const float beforeOrbital = areaBoss.ActiveBoss().currentHP;
        orbitals.Update(0.01f, owner, noEnemies, noShots, noAreas, &areaBoss);
        Check(areaBoss.ActiveBoss().currentHP < beforeOrbital,
              "Guardian Orbs damage bosses without applying knockback");
    }

    {
        Player owner;
        owner.Reset();
        WeaponManager storm;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager evolvedAreas;
        storm.Reset();
        while (storm.WeaponLevel(WeaponType::ArcBolt) < 8) storm.AddOrUpgradeWeapon(WeaponType::ArcBolt);
        storm.AddOrUpgradePassive(PassiveType::FocusCrystal, owner.stats);
        storm.EvolveWeapon(WeaponType::ArcBolt);
        for (int index = 0; index < 5; ++index)
            targets.Spawn(EnemyType::Brute, {120.0f + index * 90.0f, 0.0f});
        targets.RebuildGrid();
        storm.Update(0.3f, owner, targets, shots, evolvedAreas);
        int damaged = 0;
        for (const Enemy& enemy : targets.Items()) if (enemy.active && enemy.hp < enemy.maxHP) ++damaged;
        Check(damaged >= 3, "Storm Arc chains through distinct Spatial Grid targets");
    }

    {
        Player owner;
        owner.Reset();
        WeaponManager evolved;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager evolvedAreas;
        evolved.Reset();
        while (evolved.WeaponLevel(WeaponType::FlameRing) < 8) evolved.AddOrUpgradeWeapon(WeaponType::FlameRing);
        evolved.AddOrUpgradePassive(PassiveType::EmberCore, owner.stats);
        evolved.EvolveWeapon(WeaponType::FlameRing);
        evolved.Update(0.3f, owner, targets, shots, evolvedAreas);
        Check(evolvedAreas.ActiveCount() > 0 && evolvedAreas.Items()[0].followPlayer && evolvedAreas.Items()[0].tickInterval >= 0.25f,
              "Inferno Halo creates a persistent tick-based player halo");
    }

    {
        Player owner;
        owner.Reset();
        WeaponManager evolved;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager evolvedAreas;
        evolved.Reset();
        while (evolved.WeaponLevel(WeaponType::GuardianOrbs) < 8) evolved.AddOrUpgradeWeapon(WeaponType::GuardianOrbs);
        evolved.AddOrUpgradePassive(PassiveType::OrbitalEngine, owner.stats);
        evolved.EvolveWeapon(WeaponType::GuardianOrbs);
        evolved.Update(0.3f, owner, targets, shots, evolvedAreas);
        Check(shots.ActiveCount(ProjectileOwner::Player) == 10,
              "Celestial Guard releases a bounded radial projectile burst");
    }

    {
        Player owner;
        owner.Reset();
        WeaponManager evolved;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager evolvedAreas;
        evolved.Reset();
        while (evolved.WeaponLevel(WeaponType::SpectralFan) < 8) evolved.AddOrUpgradeWeapon(WeaponType::SpectralFan);
        evolved.AddOrUpgradePassive(PassiveType::WindSigil, owner.stats);
        evolved.EvolveWeapon(WeaponType::SpectralFan);
        evolved.Update(0.3f, owner, targets, shots, evolvedAreas);
        Check(shots.ActiveCount(ProjectileOwner::Player) == 18,
              "Phantom Barrage fires a controlled 18-projectile ring");
    }

    {
        Player owner;
        owner.Reset();
        WeaponManager evolved;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager evolvedAreas;
        evolved.Reset();
        while (evolved.WeaponLevel(WeaponType::VoidLance) < 8) evolved.AddOrUpgradeWeapon(WeaponType::VoidLance);
        evolved.AddOrUpgradePassive(PassiveType::PiercingEye, owner.stats);
        evolved.EvolveWeapon(WeaponType::VoidLance);
        targets.Spawn(EnemyType::Brute, {300.0f, 0.0f});
        targets.RebuildGrid();
        evolved.Update(0.3f, owner, targets, shots, evolvedAreas);
        const Projectile* spear = nullptr;
        for (const Projectile& shot : shots.Items())
            if (shot.active && shot.sourceWeapon == WeaponType::VoidLance) { spear = &shot; break; }
        Check(spear != nullptr && spear->remainingPierces >= 1000 && spear->radius >= 18.0f,
              "Abyss Spear is wide, long-range and effectively infinitely piercing");
    }

    {
        Player owner;
        owner.Reset();
        WeaponManager evolved;
        EnemyManager targets;
        ProjectileManager shots;
        AreaEffectManager evolvedAreas;
        evolved.Reset();
        while (evolved.WeaponLevel(WeaponType::ThunderCannon) < 8) evolved.AddOrUpgradeWeapon(WeaponType::ThunderCannon);
        evolved.AddOrUpgradePassive(PassiveType::TitanCore, owner.stats);
        evolved.EvolveWeapon(WeaponType::ThunderCannon);
        targets.Spawn(EnemyType::Brute, {300.0f, 0.0f});
        targets.RebuildGrid();
        evolved.Update(0.3f, owner, targets, shots, evolvedAreas);
        const Projectile* cannon = nullptr;
        for (const Projectile& shot : shots.Items())
            if (shot.active && shot.sourceWeapon == WeaponType::ThunderCannon) { cannon = &shot; break; }
        Check(cannon != nullptr && cannon->explosive && cannon->explosionRadius > 100.0f,
              "Tempest Cannon launches an empowered primary explosion");
    }

    {
        GameSession encounter;
        auto modes = std::make_unique<GameModeManager>();
        modes->ActivateMode(GameModeType::Survival, encounter);
        encounter.DebugSpawnBoss(BossType::FlameWyrm);
        for (int index = 0; index < 100; ++index)
            encounter.Enemies().Spawn(EnemyType::Ghoul, {500.0f + index, 200.0f});
        modes->UpdateActiveMode(0.01f, encounter);
        Check(encounter.Enemies().ActiveCount() <= 30,
              "active boss encounter caps existing and future common enemy pressure at 30");
    }

    {
        Player target;
        target.Reset();
        target.ToggleDebugInvulnerability();
        BossManager stressBoss;
        EnemyManager stressEnemies;
        ProjectileManager stressShots;
        XPOrbManager stressXP;
        ParticleManager stressParticles;
        stressBoss.Reset();
        stressBoss.SpawnDebug(BossType::VoidHeraldAscended, target.Position());
        stressBoss.Damage(stressBoss.ActiveBoss().maxHP * 0.65f, DamageSource::PlayerArea);
        for (int index = 0; index < 100; ++index) {
            stressEnemies.Spawn(EnemyType::Ghoul, {500.0f + index * 3.0f, 200.0f});
            stressXP.Spawn({300.0f + index, -100.0f}, 1);
            Projectile shot{};
            shot.position = {static_cast<float>(index), 0.0f};
            shot.velocity = {100.0f, 0.0f};
            shot.lifetime = 10.0f;
            shot.owner = ProjectileOwner::Player;
            stressShots.Spawn(shot);
        }
        stressParticles.SpawnBurst({0.0f, 0.0f}, VIOLET, 500, 120.0f);
        const auto bossStressStart = std::chrono::steady_clock::now();
        int peakBossProjectiles = 0;
        for (int frame = 0; frame < 300; ++frame) {
            stressBoss.Update(1.0f / 60.0f, target, stressShots);
            stressEnemies.Update(1.0f / 60.0f, target.Position(), stressShots);
            stressShots.Update(1.0f / 60.0f);
            stressXP.Update(1.0f / 60.0f, target.Position(), target.Radius(), 105.0f);
            stressParticles.Update(1.0f / 60.0f);
            peakBossProjectiles = std::max(peakBossProjectiles,
                                           stressShots.ActiveCount(ProjectileOwner::Boss));
        }
        const double bossStressMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - bossStressStart).count();
        std::cout << "Boss stress: final boss, 100 enemies, 100 player shots, boss shots, XP and 500 particles, "
                  << bossStressMs << " ms / 300 updates\n";
        Check(stressBoss.IsActive() && stressBoss.ActiveBoss().phase == 2 &&
                  peakBossProjectiles >= 40,
              "Ascended final phase uses enhanced double-ring patterns under stress");
    }

    {
        EnemyManager stressEnemies;
        ProjectileManager stressProjectiles;
        XPOrbManager stressXP;
        ParticleManager stressParticles;
        for (int index = 0; index < 500; ++index) {
            const float x = -1800.0f + static_cast<float>(index % 25) * 145.0f;
            const float y = -1400.0f + static_cast<float>(index / 25) * 140.0f;
            stressEnemies.Spawn(static_cast<EnemyType>(index % 5), {x, y});
            stressXP.Spawn({x + 10.0f, y}, 1);
        }
        for (int index = 0; index < 250; ++index) {
            Projectile projectile{};
            projectile.position = {static_cast<float>(index), 0.0f};
            projectile.velocity = {80.0f, 0.0f};
            projectile.lifetime = 10.0f;
            projectile.owner = index < 150 ? ProjectileOwner::Player : ProjectileOwner::Enemy;
            stressProjectiles.Spawn(projectile);
        }
        stressParticles.SpawnBurst({0.0f, 0.0f}, WHITE, 1000, 100.0f);
        stressEnemies.RebuildGrid();
        std::vector<int> candidates;
        candidates.reserve(256);
        const auto start = std::chrono::steady_clock::now();
        for (int frame = 0; frame < 120; ++frame) {
            stressEnemies.Update(1.0f / 60.0f, {0.0f, 0.0f}, stressProjectiles);
            for (const Projectile& projectile : stressProjectiles.Items()) if (projectile.active)
                stressEnemies.QueryCircle(projectile.position, 48.0f, candidates);
            stressProjectiles.Update(1.0f / 60.0f);
            stressXP.Update(1.0f / 60.0f, {0.0f, 0.0f}, 18.0f, 105.0f);
            stressParticles.Update(1.0f / 60.0f);
        }
        const double milliseconds = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        std::cout << "Stress C: 500 enemies, 250 projectiles, 500 XP, 1000 particles, "
                  << milliseconds << " ms / 120 updates\n";
        Check(stressEnemies.GridEntryCount() <= stressEnemies.ActiveCount() &&
                  stressProjectiles.ActiveCount() >= 250 && stressXP.ActiveCount() <= 500,
              "stress C remains stable with spatial queries");
    }

    {
        FloatingTextManager text;
        text.Spawn({10.0f, 10.0f}, 12.0f, false);
        text.Spawn({12.0f, 10.0f}, 8.0f, true);
        Check(text.ActiveCount() == 1, "nearby rapid damage numbers aggregate to control clutter");
        text.Update(1.0f);
        Check(text.ActiveCount() == 0, "floating damage text expires and returns to its pool");
    }

    {
        ParticleManager visualPool;
        visualPool.SpawnBurst({}, WHITE, 1600, 1.0f, ParticleKind::Spark, ParticlePriority::Low);
        visualPool.SpawnBurst({}, GOLD, 10, 1.0f, ParticleKind::Shard, ParticlePriority::Important);
        Check(visualPool.ActiveCount() == 1600, "important VFX reclaims low priority particles at the cap");
        visualPool.Reset();
        Check(visualPool.ActiveCount() == 0, "particle pool resets cleanly between runs");
    }

    {
        ScreenShakeManager shake;
        shake.Add(8.0f, 0.25f);
        Check(shake.Magnitude() > 0.0f, "impact starts centralized screen shake");
        shake.Update(0.30f);
        Check(shake.Magnitude() == 0.0f && Near(shake.Offset(1.0f).x, 0.0f),
              "screen shake decays fully without residual camera offset");
    }

    if (failures == 0) std::cout << "All Stage 11 regression checks passed.\n";
    return failures == 0 ? 0 : 1;
}
