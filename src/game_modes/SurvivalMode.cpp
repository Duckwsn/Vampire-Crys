#include "game_modes/SurvivalMode.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "gameplay/GameSession.h"

namespace {
constexpr std::array<float, 3> EncounterTimes{{300.0f, 600.0f, 900.0f}};
constexpr std::array<BossType, 3> EncounterTypes{{
    BossType::FlameWyrm,
    BossType::VoidHerald,
    BossType::VoidHeraldAscended
}};
constexpr std::array<float, 3> MinibossTimes{{195.0f, 465.0f, 755.0f}};
constexpr std::array<EnemyType, 3> MinibossTypes{{EnemyType::GraveWarden,
                                                  EnemyType::VoidStalker,
                                                  EnemyType::GraveWarden}};

int EncounterIndex(BossType type) {
    return type == BossType::FlameWyrm ? 0 :
           (type == BossType::VoidHerald ? 1 : 2);
}
} // namespace

void SurvivalMode::Reset() {
    director_.Reset();
    runSeed_ = static_cast<unsigned int>(GetRandomValue(1, 0x3fffffff));
    events_.Reset(runSeed_);
    encounterDue_ = {};
    encounterCompleted_ = {};
    encounterDelay_ = 0.0f;
    finalBossDefeated_ = false;
    minibossDue_ = {};
    minibossCompleted_ = {};
    specialDebugIndex_ = 0;
    config_ = ResolveSurvivalConfig(config_);
    modifiers_ = BuildRunModifiers(config_);
    endlessCycle_ = 1;
    endlessBossDue_ = false;
    nextEndlessBoss_ = 1200.0f;
    nextBonusMiniboss_ = modifiers_.bossHunter ? 135.0f : 1050.0f;
    endlessBossSequence_ = 0;
}

void SurvivalMode::OnRunStart(GameSession& session) {
    session.SetManualCombat(false);
    session.ConfigureBossScaling(modifiers_.bossHp, modifiers_.bossDamage);
}

void SurvivalMode::Update(float deltaTime, GameSession& session) {
    session.AdvanceRunTime(deltaTime, config_.endless ? std::numeric_limits<float>::max()
                                                       : director_.RunDuration());
    endlessCycle_ = config_.endless ? std::max(1, static_cast<int>(session.ElapsedTime() / 900.0f) + 1) : 1;
    modifiers_ = BuildRunModifiers(config_, endlessCycle_);
    session.SetEndlessCyclesCompleted(config_.endless ? static_cast<int>(session.ElapsedTime() / 900.0f) : 0);
    encounterDelay_ = std::max(0.0f, encounterDelay_ - deltaTime);
    ScheduleDueEncounters(session.ElapsedTime());
    TryStartNextEncounter(session);

    for (std::size_t index = 0; index < MinibossTimes.size(); ++index)
        if (session.ElapsedTime() >= MinibossTimes[index]) minibossDue_[index] = true;
    TryStartMiniboss(session);

    const bool minibossActive = session.Enemies().HasActiveMiniboss();
    const bool pendingBoss = PendingEncounterCount() > 0;
    const bool eventBlocked = session.Bosses().IsActive() || minibossActive ||
                              director_.ActiveSpecialWave() != SpecialWaveType::None || pendingBoss;
    events_.Update(deltaTime, session.ElapsedTime(), eventBlocked,
                   session.GetPlayer().Position(), session.Enemies(), modifiers_.eventInterval,
                   config_.endless ? 1000000 : 3,
                   modifiers_.preferBloodMoon ? SurvivalEventType::BloodMoon : SurvivalEventType::None);
    const SurvivalEventModifiers eventModifiers = events_.Modifiers();
    session.SetEnemySpeedMultiplier(eventModifiers.enemySpeedMultiplier);

    if (session.Bosses().IsActive() && session.Enemies().ActiveCount() > 30)
        session.Enemies().CullToLimit(30);
    if (config_.endless || session.ElapsedTime() < director_.RunDuration()) {
        const WaveRuntimeModifiers modifiers{eventModifiers.spawnIntervalMultiplier,
                                             eventModifiers.eliteChanceMultiplier,
                                             eventModifiers.normalPressureMultiplier,
                                             eventModifiers.forceSwarmComposition || modifiers_.forceSwarm,
                                             modifiers_.enemyHp, modifiers_.enemyDamage,
                                             modifiers_.enemySpeed, modifiers_.specialWaveInterval,
                                             modifiers_.forceDurable};
        WaveRuntimeModifiers effective = modifiers;
        effective.spawnIntervalMultiplier *= modifiers_.spawnInterval;
        effective.eliteChanceMultiplier *= modifiers_.eliteChance;
        effective.pressureMultiplier *= modifiers_.spawnPressure;
        director_.Update(deltaTime, session.ElapsedTime(), session.GetPlayer().Position(), 1.0f,
                         session.Enemies(), session.Bosses().IsActive(), effective,
                         events_.IsActive() || minibossActive || pendingBoss);
    }
}

void SurvivalMode::AfterSharedUpdate(GameSession& session) {
    if (session.HasMinibossDeathThisFrame()) {
        for (std::size_t index = 0; index < MinibossTypes.size(); ++index)
            if (minibossDue_[index] && !minibossCompleted_[index] &&
                MinibossTypes[index] == session.MinibossDeathTypeThisFrame()) {
                minibossCompleted_[index] = true; break;
            }
    }
    if (session.HasBossDeathThisFrame()) {
        const BossDeathEvent& death = session.BossDeathThisFrame();
        encounterCompleted_[static_cast<std::size_t>(EncounterIndex(death.type))] = true;
        encounterDelay_ = 1.5f;
        if (death.type == BossType::VoidHeraldAscended && !config_.endless) finalBossDefeated_ = true;
        else session.GrantEvolutionOpportunity(death.position);
    }
}

ModeHUDData SurvivalMode::HUDData() const {
    const WaveSegment& wave = director_.CurrentWave();
    ModeHUDData data{GameModeType::Survival, "Wave", wave.name.c_str(), director_.RunDuration(),
                     director_.CurrentWaveIndex() + 1, PendingEncounterCount(), wave.spawnInterval,
                     wave.spawnCount, wave.enemyCap};
    data.eventName = events_.Name();
    data.eventSubtitle = events_.Subtitle();
    data.eventRemaining = events_.Remaining();
    data.eventActive = events_.IsActive();
    data.specialWaveName = director_.ActiveSpecialWaveName();
    data.specialWaveRemaining = director_.SpecialWaveRemaining();
    data.threatBudget = director_.ThreatBudget();
    data.runSeed = runSeed_;
    data.minibossName = "NONE";
    data.difficulty = config_.difficulty;
    data.challenge = config_.challenge;
    data.ascension = config_.ascension;
    data.endless = config_.endless;
    data.endlessCycle = endlessCycle_;
    data.mutatorCount = config_.MutatorCount();
    data.scoreMultiplier = modifiers_.scoreMultiplier;
    return data;
}

void SurvivalMode::DebugSkipTime(float seconds, GameSession& session) {
    session.AdvanceRunTime(seconds, config_.endless ? std::numeric_limits<float>::max()
                                                     : director_.RunDuration());
    ScheduleDueEncounters(session.ElapsedTime());
}

void SurvivalMode::DebugSpawnEnemies(int count, GameSession& session) {
    director_.SpawnStress(count, session.ElapsedTime(), session.GetPlayer().Position(),
                          session.Enemies());
}

int SurvivalMode::PendingEncounterCount() const {
    int count = endlessBossDue_ ? 1 : 0;
    for (std::size_t index = 0; index < encounterDue_.size(); ++index)
        if (encounterDue_[index] && !encounterCompleted_[index]) ++count;
    return count;
}

void SurvivalMode::ScheduleDueEncounters(float elapsedTime) {
    for (std::size_t index = 0; index < EncounterTimes.size(); ++index)
        if (elapsedTime >= EncounterTimes[index]) encounterDue_[index] = true;
    if (config_.endless && elapsedTime >= nextEndlessBoss_) endlessBossDue_ = true;
}

void SurvivalMode::TryStartNextEncounter(GameSession& session) {
    if (session.Bosses().IsActive() ||
        events_.IsActive() || director_.ActiveSpecialWave() != SpecialWaveType::None ||
        session.Bosses().HasDeathEvent() ||
        encounterDelay_ > 0.0f) return;
    for (std::size_t index = 0; index < EncounterTypes.size(); ++index) {
        if (!encounterDue_[index] || encounterCompleted_[index]) continue;
        // Boss milestones own the arena. An unfinished miniboss retreats without reward.
        if (session.Enemies().HasActiveMiniboss()) {
            session.Enemies().DespawnMiniboss();
            session.Projectiles().ClearOwner(ProjectileOwner::Enemy);
        }
        session.ConfigureBossScaling(modifiers_.bossHp, modifiers_.bossDamage);
        session.Bosses().StartEncounter(EncounterTypes[index], session.GetPlayer().Position());
        return;
    }
    if (config_.endless && endlessBossDue_) {
        if (session.Enemies().HasActiveMiniboss()) session.Enemies().DespawnMiniboss();
        const std::array<BossType, 3> cycleBosses{{BossType::FlameWyrm, BossType::VoidHerald,
                                                   BossType::VoidHeraldAscended}};
        session.ConfigureBossScaling(modifiers_.bossHp, modifiers_.bossDamage);
        if (session.Bosses().StartEncounter(cycleBosses[static_cast<std::size_t>(endlessBossSequence_ % 3)],
                                            session.GetPlayer().Position())) {
            ++endlessBossSequence_;
            endlessBossDue_ = false;
            nextEndlessBoss_ += 300.0f;
        }
    }
}

void SurvivalMode::TryStartMiniboss(GameSession& session) {
    if (session.Bosses().IsActive() || session.Enemies().HasActiveMiniboss() ||
        events_.IsActive() || director_.ActiveSpecialWave() != SpecialWaveType::None ||
        PendingEncounterCount() > 0) return;
    const float elapsed = session.ElapsedTime();
    const float nextBoss = elapsed < 300.0f ? 300.0f :
                           (elapsed < 600.0f ? 600.0f :
                           (elapsed < 900.0f ? 900.0f :
                            (std::floor(elapsed / 300.0f) + 1.0f) * 300.0f));
    if (nextBoss - elapsed < 38.0f) return;
    for (std::size_t index = 0; index < MinibossTypes.size(); ++index) {
        if (!minibossDue_[index] || minibossCompleted_[index]) continue;
        const float angle = (55.0f + index * 113.0f) * DEG2RAD;
        const Vector2 player = session.GetPlayer().Position();
        if (session.Enemies().SpawnMiniboss(MinibossTypes[index],
                {std::clamp(player.x + std::cos(angle) * 720.0f, -1940.0f, 1940.0f),
                 std::clamp(player.y + std::sin(angle) * 720.0f, -1940.0f, 1940.0f)},
                (1.0f + elapsed / 1500.0f) * modifiers_.minibossHp)) return;
    }
    const bool extraDue = (modifiers_.bossHunter || config_.endless) && elapsed >= nextBonusMiniboss_;
    if (extraDue) {
        const EnemyType type = static_cast<int>(elapsed / 240.0f) % 2 == 0
                                   ? EnemyType::GraveWarden : EnemyType::VoidStalker;
        const Vector2 player = session.GetPlayer().Position();
        if (session.Enemies().SpawnMiniboss(type, {std::clamp(player.x + 680.0f, -1940.0f, 1940.0f), player.y},
                                            (1.0f + elapsed / 1500.0f) * modifiers_.minibossHp))
            nextBonusMiniboss_ = elapsed + (modifiers_.bossHunter ? 210.0f : 240.0f);
    }
}

void SurvivalMode::DebugForceSpecialWave() {
    director_.ForceSpecialWave(static_cast<SpecialWaveType>(1 + specialDebugIndex_ % 5));
    ++specialDebugIndex_;
}

void SurvivalMode::DebugSpawnMiniboss(GameSession& session, bool stalker) {
    const Vector2 player = session.GetPlayer().Position();
    session.Enemies().SpawnMiniboss(stalker ? EnemyType::VoidStalker : EnemyType::GraveWarden,
                                    {player.x + 620.0f, player.y}, 1.0f);
}
