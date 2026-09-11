#include "systems/WaveDirector.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <filesystem>

#include <nlohmann/json.hpp>

#include "gameplay/Balance.h"
#include "systems/EnemyManager.h"

namespace {
EnemyType TypeFromName(const std::string& name) {
    if (name == "ghoul") return EnemyType::Ghoul;
    if (name == "swarmer") return EnemyType::Swarmer;
    if (name == "brute") return EnemyType::Brute;
    if (name == "cultist") return EnemyType::Cultist;
    if (name == "bomber") return EnemyType::Bomber;
    if (name == "shielded_acolyte") return EnemyType::ShieldedAcolyte;
    if (name == "wraith") return EnemyType::Wraith;
    if (name == "necromancer") return EnemyType::Necromancer;
    TraceLog(LOG_WARNING, "Invalid enemy type in wave config: %s", name.c_str());
    return EnemyType::Count;
}

float Interpolate(float time, float middleTime, float endTime,
                  float startValue, float middleValue, float endValue) {
    if (time <= middleTime)
        return startValue + (middleValue - startValue) * std::clamp(time / middleTime, 0.0f, 1.0f);
    return middleValue + (endValue - middleValue) *
                             std::clamp((time - middleTime) / (endTime - middleTime), 0.0f, 1.0f);
}
} // namespace

WaveDirector::WaveDirector() { LoadConfiguration(); }

void WaveDirector::Reset() {
    spawnTimer_ = 0.0f;
    currentWaveIndex_ = 0;
    difficulty_ = {};
    specialWave_ = forcedSpecialWave_ = SpecialWaveType::None;
    specialWaveTimer_ = threatBudget_ = 0.0f;
    nextSpecialWave_ = 78.0f;
    specialSequence_ = specialBurst_ = 0;
    runtimeModifiers_ = {};
}

void WaveDirector::Update(float deltaTime, float elapsedTime, Vector2 playerPosition,
                          float cameraZoom, EnemyManager& enemies, bool bossEncounterActive,
                          WaveRuntimeModifiers modifiers, bool encountersBlocked) {
    SelectWave(elapsedTime);
    difficulty_ = CalculateDifficulty(elapsedTime);
    runtimeModifiers_ = modifiers;
    const float nextBoss = elapsedTime < 300.0f ? 300.0f :
                           (elapsedTime < 600.0f ? 600.0f :
                           (elapsedTime < 900.0f ? 900.0f :
                            (std::floor(elapsedTime / 300.0f) + 1.0f) * 300.0f));
    if (specialWave_ == SpecialWaveType::None && !bossEncounterActive && !encountersBlocked &&
        (forcedSpecialWave_ != SpecialWaveType::None || elapsedTime >= nextSpecialWave_) &&
        nextBoss - elapsedTime > 24.0f) StartSpecialWave(elapsedTime);
    if (specialWave_ != SpecialWaveType::None) {
        specialWaveTimer_ -= deltaTime;
        spawnTimer_ -= deltaTime;
        if (specialWaveTimer_ <= 0.0f) {
            specialWave_ = SpecialWaveType::None;
            nextSpecialWave_ = elapsedTime + (115.0f + static_cast<float>((specialSequence_ * 17) % 31)) *
                                               modifiers.specialWaveIntervalMultiplier;
            spawnTimer_ = 1.0f;
        } else if (spawnTimer_ <= 0.0f) {
            SpawnSpecialGroup(playerPosition, cameraZoom, enemies);
            spawnTimer_ = specialWave_ == SpecialWaveType::BomberRush ? 1.25f : 0.72f;
        }
        return;
    }
    spawnTimer_ -= deltaTime;
    const WaveSegment& wave = CurrentWave();
    const int effectiveCap = bossEncounterActive ? std::min(wave.enemyCap, 30) : wave.enemyCap;
    if (spawnTimer_ > 0.0f || enemies.ActiveCount() >= effectiveCap) return;

    const int effectiveCount = bossEncounterActive ? 1 :
        std::max(1, static_cast<int>(std::round(wave.spawnCount * modifiers.pressureMultiplier)));
    for (int index = 0; index < effectiveCount && enemies.ActiveCount() < effectiveCap; ++index) {
        EnemyType type = SelectEnemyType(wave, enemies);
        if (modifiers.forceSwarmComposition && GetRandomValue(0, 99) < 82) type = EnemyType::Swarmer;
        else if (modifiers.forceDurableComposition && GetRandomValue(0, 99) < 65)
            type = GetRandomValue(0, 1) == 0 ? EnemyType::Brute : EnemyType::ShieldedAcolyte;
        if (type == EnemyType::Count) break;
        enemies.Spawn(type, SpawnPosition(playerPosition, cameraZoom),
                      difficulty_.hp * modifiers.enemyHpMultiplier,
                      difficulty_.damage * modifiers.enemyDamageMultiplier,
                      difficulty_.speed * modifiers.enemySpeedMultiplier, difficulty_.xp,
                      RollElite(difficulty_.eliteChance * modifiers.eliteChanceMultiplier),
                      SpawnSource::Director,
                      elapsedTime > 210.0f && GetRandomValue(0, 99) < 9
                          ? static_cast<EnemyVariant>(1 + GetRandomValue(0, 2))
                          : EnemyVariant::None);
    }
    spawnTimer_ = wave.spawnInterval * modifiers.spawnIntervalMultiplier *
                  (bossEncounterActive ? 5.0f : 1.0f);
}

void WaveDirector::ForceSpecialWave(SpecialWaveType type) { forcedSpecialWave_ = type; }

const char* WaveDirector::ActiveSpecialWaveName() const {
    switch (specialWave_) {
        case SpecialWaveType::SwarmWave: return "SWARM WAVE";
        case SpecialWaveType::BruteWall: return "BRUTE WALL";
        case SpecialWaveType::CultistCircle: return "CULTIST CIRCLE";
        case SpecialWaveType::BomberRush: return "BOMBER RUSH";
        case SpecialWaveType::MixedAssault: return "MIXED ASSAULT";
        default: return "NONE";
    }
}

void WaveDirector::StartSpecialWave(float) {
    specialWave_ = forcedSpecialWave_ != SpecialWaveType::None
                       ? forcedSpecialWave_
                       : static_cast<SpecialWaveType>(1 + specialSequence_ % 5);
    forcedSpecialWave_ = SpecialWaveType::None;
    specialWaveTimer_ = 12.0f;
    spawnTimer_ = 0.0f;
    specialBurst_ = 0;
    ++specialSequence_;
}

float WaveDirector::ThreatCost(EnemyType type) const {
    if (type == EnemyType::Swarmer || type == EnemyType::BoneMinion) return 1.0f;
    if (type == EnemyType::Ghoul) return 2.0f;
    if (type == EnemyType::Cultist || type == EnemyType::Bomber || type == EnemyType::Wraith) return 4.0f;
    if (type == EnemyType::Brute || type == EnemyType::ShieldedAcolyte) return 6.0f;
    return 8.0f;
}

void WaveDirector::SpawnSpecialGroup(Vector2 playerPosition, float cameraZoom,
                                     EnemyManager& enemies) {
    threatBudget_ = std::min(30.0f, 14.0f + difficulty_.hp * 6.0f);
    int count = 0;
    while (threatBudget_ >= 1.0f && count < 16 && enemies.ActiveCount() < CurrentWave().enemyCap) {
        EnemyType type = EnemyType::Ghoul;
        switch (specialWave_) {
            case SpecialWaveType::SwarmWave: type = EnemyType::Swarmer; break;
            case SpecialWaveType::BruteWall: type = count % 3 == 0 ? EnemyType::ShieldedAcolyte : EnemyType::Brute; break;
            case SpecialWaveType::CultistCircle: type = count % 4 == 0 ? EnemyType::Necromancer : EnemyType::Cultist; break;
            case SpecialWaveType::BomberRush: type = count % 3 == 0 ? EnemyType::Wraith : EnemyType::Bomber; break;
            case SpecialWaveType::MixedAssault:
                type = static_cast<EnemyType>((specialBurst_ + count) % 8); break;
            default: break;
        }
        const float cost = ThreatCost(type);
        if (cost > threatBudget_) break;
        Vector2 position = SpawnPosition(playerPosition, cameraZoom);
        if (specialWave_ == SpecialWaveType::CultistCircle) {
            const float angle = count * 45.0f * DEG2RAD;
            position = {playerPosition.x + std::cos(angle) * 720.0f,
                        playerPosition.y + std::sin(angle) * 720.0f};
        } else if (specialWave_ == SpecialWaveType::BruteWall) {
            position.x = playerPosition.x + (specialBurst_ % 2 == 0 ? -760.0f : 760.0f);
            position.y = playerPosition.y - 330.0f + count * 75.0f;
        } else if (specialWave_ == SpecialWaveType::BomberRush) {
            position.x = playerPosition.x + (specialBurst_ % 2 == 0 ? -760.0f : 760.0f);
        }
        position.x = std::clamp(position.x, Balance::Arena.x + 35.0f,
                               Balance::Arena.x + Balance::Arena.width - 35.0f);
        position.y = std::clamp(position.y, Balance::Arena.y + 35.0f,
                               Balance::Arena.y + Balance::Arena.height - 35.0f);
        if (!enemies.Spawn(type, position,
                           difficulty_.hp * runtimeModifiers_.enemyHpMultiplier,
                           difficulty_.damage * runtimeModifiers_.enemyDamageMultiplier,
                           difficulty_.speed * runtimeModifiers_.enemySpeedMultiplier,
                           difficulty_.xp, false, SpawnSource::SpecialWave)) break;
        threatBudget_ -= cost;
        ++count;
    }
    ++specialBurst_;
}

void WaveDirector::SpawnStress(int count, float elapsedTime, Vector2 playerPosition,
                               EnemyManager& enemies) {
    SelectWave(elapsedTime);
    difficulty_ = CalculateDifficulty(elapsedTime);
    for (int index = 0; index < count; ++index) {
        const EnemyType type = static_cast<EnemyType>(index % static_cast<int>(EnemyType::Count));
        if (!enemies.Spawn(type, SpawnPosition(playerPosition, 1.0f), difficulty_.hp,
                           difficulty_.damage, difficulty_.speed, difficulty_.xp,
                           RollElite(difficulty_.eliteChance))) break;
    }
    enemies.RebuildGrid();
}

const WaveSegment& WaveDirector::CurrentWave() const { return waves_[static_cast<std::size_t>(currentWaveIndex_)]; }

void WaveDirector::LoadConfiguration() {
    LoadSafeDefaults();
    const std::filesystem::path executableConfig =
        std::filesystem::path(GetApplicationDirectory()) / "config" / "waves.json";
    std::ifstream file(executableConfig);
    if (!file) {
        file.open("config/waves.json");
    }
    if (!file) {
        TraceLog(LOG_WARNING, "waves.json missing; safe defaults loaded");
        return;
    }
    try {
        const nlohmann::json root = nlohmann::json::parse(file);
        std::vector<WaveSegment> parsed;
        for (const auto& item : root.at("waves")) {
            WaveSegment wave{};
            wave.name = item.at("name").get<std::string>();
            wave.start = item.at("start").get<float>();
            wave.end = item.at("end").get<float>();
            if (!std::isfinite(wave.start) || !std::isfinite(wave.end) || wave.start < 0.0f ||
                wave.end <= wave.start) throw std::runtime_error("wave has invalid time range");
            wave.spawnInterval = std::max(0.05f, item.at("interval").get<float>());
            wave.spawnCount = std::max(1, item.at("count").get<int>());
            wave.enemyCap = std::clamp(item.at("cap").get<int>(), 1, Balance::EnemyPoolSize);
            wave.maxCultists = std::clamp(item.value("max_cultists", wave.enemyCap), 0, wave.enemyCap);
            wave.maxBombers = std::clamp(item.value("max_bombers", wave.enemyCap), 0, wave.enemyCap);
            for (auto entry = item.at("weights").begin(); entry != item.at("weights").end(); ++entry) {
                const EnemyType type = TypeFromName(entry.key());
                if (type != EnemyType::Count)
                    wave.weights[static_cast<std::size_t>(type)] =
                        std::max(0.0f, entry.value().get<float>());
            }
            float totalWeight = 0.0f;
            for (float weight : wave.weights) totalWeight += weight;
            if (!std::isfinite(totalWeight) || totalWeight <= 0.0f)
                throw std::runtime_error("wave has no positive enemy weights");
            parsed.push_back(wave);
        }
        if (parsed.empty()) throw std::runtime_error("waves array is empty");
        waves_ = std::move(parsed);
        runDuration_ = root.value("run_duration", DefaultRunDuration);
        if (!std::isfinite(runDuration_) || runDuration_ < 60.0f || runDuration_ > 7200.0f)
            throw std::runtime_error("run_duration outside safe range");
        loadedExternalConfig_ = true;
        TraceLog(LOG_INFO, "Wave config loaded: %i phases, %.0fs run", static_cast<int>(waves_.size()), runDuration_);
    } catch (const std::exception& error) {
        TraceLog(LOG_WARNING, "Invalid waves.json (%s); safe defaults loaded", error.what());
        LoadSafeDefaults();
    }
}

void WaveDirector::LoadSafeDefaults() {
    waves_.clear();
    WaveSegment wave{"Safe Early", 0, 900, 0.6f, 2, 300, 24, 12, {}};
    wave.weights[0] = 50; wave.weights[1] = 25; wave.weights[2] = 12;
    wave.weights[3] = 8; wave.weights[4] = 5;
    waves_.push_back(wave);
    runDuration_ = DefaultRunDuration;
    loadedExternalConfig_ = false;
}

void WaveDirector::SelectWave(float elapsedTime) {
    for (std::size_t index = 0; index < waves_.size(); ++index)
        if (elapsedTime >= waves_[index].start && elapsedTime < waves_[index].end) {
            currentWaveIndex_ = static_cast<int>(index);
            return;
        }
    currentWaveIndex_ = static_cast<int>(waves_.size() - 1);
}

DifficultyState WaveDirector::CalculateDifficulty(float elapsedTime) const {
    const float time = std::clamp(elapsedTime, 0.0f, runDuration_);
    DifficultyState state{};
    state.hp = Interpolate(time, 300, 900, 1.0f, 1.25f, 2.0f);
    state.damage = Interpolate(time, 300, 900, 1.0f, 1.10f, 1.45f);
    state.speed = 1.0f + 0.15f * (time / runDuration_);
    state.xp = 1.0f + 0.60f * (time / runDuration_);
    if (time >= 600.0f) state.eliteChance = 0.05f + 0.03f * ((time - 600.0f) / 300.0f);
    else if (time >= 300.0f) state.eliteChance = 0.02f + 0.02f * ((time - 300.0f) / 300.0f);
    else if (time >= 180.0f) state.eliteChance = 0.01f;
    return state;
}

EnemyType WaveDirector::SelectEnemyType(const WaveSegment& wave, const EnemyManager& enemies) const {
    auto allowedWeight = [&](EnemyType type) {
        if (type == EnemyType::Cultist && enemies.ActiveCount(type) >= wave.maxCultists) return 0.0f;
        if (type == EnemyType::Bomber && enemies.ActiveCount(type) >= wave.maxBombers) return 0.0f;
        return wave.weights[static_cast<std::size_t>(type)];
    };
    float total = 0.0f;
    for (int raw = 0; raw < static_cast<int>(EnemyType::Count); ++raw)
        total += allowedWeight(static_cast<EnemyType>(raw));
    if (total <= 0.0f) return EnemyType::Count;
    float roll = static_cast<float>(GetRandomValue(0, 1000000)) / 1000000.0f * total;
    for (int raw = 0; raw < static_cast<int>(EnemyType::Count); ++raw) {
        const EnemyType type = static_cast<EnemyType>(raw);
        roll -= allowedWeight(type);
        if (roll <= 0.0f) return type;
    }
    return EnemyType::Ghoul;
}

Vector2 WaveDirector::SpawnPosition(Vector2 playerPosition, float cameraZoom) const {
    const float width = static_cast<float>(std::max(GetScreenWidth(), Balance::ScreenWidth));
    const float height = static_cast<float>(std::max(GetScreenHeight(), Balance::ScreenHeight));
    const float safeZoom = std::max(0.1f, cameraZoom);
    const float inner = std::sqrt(width * width + height * height) * 0.5f / safeZoom + 120.0f;
    const float outer = inner + 420.0f;
    for (int attempt = 0; attempt < 16; ++attempt) {
        const float angle = static_cast<float>(GetRandomValue(0, 3599)) * 0.1f * DEG2RAD;
        const float distance = inner + (outer - inner) *
            (static_cast<float>(GetRandomValue(0, 1000)) / 1000.0f);
        Vector2 result{playerPosition.x + std::cos(angle) * distance,
                       playerPosition.y + std::sin(angle) * distance};
        result.x = std::clamp(result.x, Balance::Arena.x + 35.0f,
                              Balance::Arena.x + Balance::Arena.width - 35.0f);
        result.y = std::clamp(result.y, Balance::Arena.y + 35.0f,
                              Balance::Arena.y + Balance::Arena.height - 35.0f);
        const float dx = result.x - playerPosition.x;
        const float dy = result.y - playerPosition.y;
        if (dx * dx + dy * dy >= inner * inner) return result;
    }
    Vector2 towardCenter{-playerPosition.x, -playerPosition.y};
    const float length = std::sqrt(std::max(1.0f, towardCenter.x * towardCenter.x +
                                                  towardCenter.y * towardCenter.y));
    return {playerPosition.x + towardCenter.x / length * inner,
            playerPosition.y + towardCenter.y / length * inner};
}

bool WaveDirector::RollElite(float chance) const {
    return GetRandomValue(0, 99999) < static_cast<int>(std::clamp(chance, 0.0f, 0.25f) * 100000.0f);
}
