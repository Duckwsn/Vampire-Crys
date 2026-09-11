#include "game_modes/SurvivalEndgame.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr std::uint16_t Bit(MutatorId id) {
    return static_cast<std::uint16_t>(1u << static_cast<unsigned int>(id));
}

constexpr std::array<MutatorDefinition, static_cast<std::size_t>(MutatorId::Count)> Mutators{{
    {MutatorId::GlassCannon, "GLASS CANNON", "+40% player damage, -40% max HP.", 2},
    {MutatorId::HyperHorde, "HYPER HORDE", "Faster, denser spawns with weaker enemies.", 2},
    {MutatorId::Titanic, "TITANIC", "Fewer enemies with greatly increased durability.", 2},
    {MutatorId::EliteInfestation, "ELITE INFESTATION", "Far more elites, slightly less normal pressure.", 3},
    {MutatorId::BloodRush, "BLOOD RUSH", "Player and enemies move faster.", 1},
    {MutatorId::ScarceRecovery, "SCARCE RECOVERY", "Health drops are reduced, never removed.", 2},
    {MutatorId::Eventful, "EVENTFUL", "Survival Events return much sooner.", 2},
    {MutatorId::ChaoticWaves, "CHAOTIC WAVES", "Special waves occur more frequently.", 2},
}};

constexpr std::array<ChallengeDefinition, static_cast<std::size_t>(ChallengeId::Count)> Challenges{{
    {ChallengeId::None, "STANDARD", "Custom Survival rules.", DifficultyTier::Normal, 0, 0},
    {ChallengeId::TheSwarm, "THE SWARM", "Weak bodies flood the arena.", DifficultyTier::Hard, 2,
     static_cast<std::uint16_t>(Bit(MutatorId::HyperHorde) | Bit(MutatorId::ScarceRecovery) | Bit(MutatorId::ChaoticWaves))},
    {ChallengeId::Titans, "TITANS", "Durable enemies and heavier frontline composition.", DifficultyTier::Hard, 3,
     Bit(MutatorId::Titanic)},
    {ChallengeId::NightOfElites, "NIGHT OF ELITES", "Elite-heavy Survival with controlled normal pressure.", DifficultyTier::Nightmare, 2,
     Bit(MutatorId::EliteInfestation)},
    {ChallengeId::BloodMoon, "BLOOD MOON", "Blood Moon events recur throughout the run.", DifficultyTier::Hard, 4,
     static_cast<std::uint16_t>(Bit(MutatorId::Eventful) | Bit(MutatorId::ScarceRecovery))},
    {ChallengeId::BossHunter, "BOSS HUNTER", "Stronger bosses and additional miniboss pressure.", DifficultyTier::Nightmare, 3,
     Bit(MutatorId::ChaoticWaves)},
    {ChallengeId::FragilePower, "FRAGILE POWER", "Extreme offense with very low survivability.", DifficultyTier::Nightmare, 1,
     static_cast<std::uint16_t>(Bit(MutatorId::GlassCannon) | Bit(MutatorId::BloodRush))},
}};
}

void SurvivalRunConfig::Sanitize() {
    const int difficultyRaw = std::clamp(static_cast<int>(difficulty), 0,
                                         static_cast<int>(DifficultyTier::Count) - 1);
    difficulty = static_cast<DifficultyTier>(difficultyRaw);
    ascension = std::clamp(ascension, 0, 10);
    const int challengeRaw = std::clamp(static_cast<int>(challenge), 0,
                                        static_cast<int>(ChallengeId::Count) - 1);
    challenge = static_cast<ChallengeId>(challengeRaw);
    mutatorMask &= static_cast<std::uint16_t>((1u << static_cast<unsigned int>(MutatorId::Count)) - 1u);
    if (MutatorCount() > 3) {
        int kept = 0;
        std::uint16_t limited = 0;
        for (int raw = 0; raw < static_cast<int>(MutatorId::Count) && kept < 3; ++raw)
            if ((mutatorMask & Bit(static_cast<MutatorId>(raw))) != 0) {
                limited |= Bit(static_cast<MutatorId>(raw)); ++kept;
            }
        mutatorMask = limited;
    }
}

int SurvivalRunConfig::MutatorCount() const {
    int count = 0;
    for (int raw = 0; raw < static_cast<int>(MutatorId::Count); ++raw)
        if ((mutatorMask & Bit(static_cast<MutatorId>(raw))) != 0) ++count;
    return count;
}

bool SurvivalRunConfig::HasMutator(MutatorId id) const { return (mutatorMask & Bit(id)) != 0; }

bool SurvivalRunConfig::ToggleMutator(MutatorId id) {
    if (challenge != ChallengeId::None) return false;
    const std::uint16_t bit = Bit(id);
    if ((mutatorMask & bit) != 0) { mutatorMask &= static_cast<std::uint16_t>(~bit); return true; }
    if (MutatorCount() >= 3) return false;
    if ((id == MutatorId::HyperHorde && HasMutator(MutatorId::Titanic)) ||
        (id == MutatorId::Titanic && HasMutator(MutatorId::HyperHorde))) return false;
    mutatorMask |= bit;
    return true;
}

const MutatorDefinition& GetMutatorDefinition(MutatorId id) {
    return Mutators[static_cast<std::size_t>(id)];
}
const ChallengeDefinition& GetChallengeDefinition(ChallengeId id) {
    return Challenges[static_cast<std::size_t>(id)];
}
const char* DifficultyName(DifficultyTier tier) {
    switch (tier) {
        case DifficultyTier::Hard: return "HARD";
        case DifficultyTier::Nightmare: return "NIGHTMARE";
        default: return "NORMAL";
    }
}
const char* ChallengeName(ChallengeId id) { return GetChallengeDefinition(id).name; }

SurvivalRunConfig ResolveSurvivalConfig(SurvivalRunConfig config) {
    config.Sanitize();
    if (config.challenge != ChallengeId::None) {
        const ChallengeDefinition& challenge = GetChallengeDefinition(config.challenge);
        config.difficulty = challenge.difficulty;
        config.ascension = challenge.ascension;
        config.mutatorMask = challenge.mutatorMask;
        config.endless = false;
    }
    return config;
}

RunModifiers BuildRunModifiers(const SurvivalRunConfig& unresolved, int endlessCycle) {
    const SurvivalRunConfig config = ResolveSurvivalConfig(unresolved);
    RunModifiers result{};
    if (config.difficulty == DifficultyTier::Hard) {
        result.enemyHp = 1.20f; result.enemyDamage = 1.15f; result.spawnInterval = 0.90f;
        result.eliteChance = 1.30f; result.minibossHp = 1.18f; result.bossHp = 1.22f;
        result.bossDamage = 1.12f; result.scoreMultiplier = 1.25f;
    } else if (config.difficulty == DifficultyTier::Nightmare) {
        result.enemyHp = 1.42f; result.enemyDamage = 1.30f; result.spawnInterval = 0.82f;
        result.spawnPressure = 1.10f; result.eliteChance = 1.70f; result.minibossHp = 1.38f;
        result.bossHp = 1.48f; result.bossDamage = 1.25f; result.healthDropChance = 0.80f;
        result.scoreMultiplier = 1.60f;
    }
    const float ascension = static_cast<float>(config.ascension);
    result.enemyHp *= 1.0f + 0.025f * ascension;
    result.enemyDamage *= 1.0f + 0.018f * ascension;
    result.spawnInterval *= std::max(0.82f, 1.0f - 0.012f * ascension);
    result.eliteChance *= 1.0f + 0.06f * ascension;
    result.minibossHp *= 1.0f + 0.035f * ascension;
    result.bossHp *= 1.0f + 0.03f * ascension;
    result.scoreMultiplier *= 1.0f + 0.05f * ascension;
    if (config.ascension >= 4) result.eventInterval *= 0.88f;
    if (config.ascension >= 6) result.spawnPressure *= 1.08f;
    if (config.ascension >= 8) result.specialWaveInterval *= 0.84f;

    if (config.HasMutator(MutatorId::GlassCannon)) { result.playerDamage *= 1.40f; result.playerHp *= 0.60f; result.scoreMultiplier *= 1.18f; }
    if (config.HasMutator(MutatorId::HyperHorde)) { result.spawnInterval *= 0.62f; result.spawnPressure *= 1.35f; result.enemyHp *= 0.68f; result.scoreMultiplier *= 1.16f; }
    if (config.HasMutator(MutatorId::Titanic)) { result.enemyHp *= 2.15f; result.spawnInterval *= 1.40f; result.spawnPressure *= 0.62f; result.scoreMultiplier *= 1.18f; }
    if (config.HasMutator(MutatorId::EliteInfestation)) { result.eliteChance *= 3.2f; result.spawnPressure *= 0.84f; result.scoreMultiplier *= 1.28f; }
    if (config.HasMutator(MutatorId::BloodRush)) { result.enemySpeed *= 1.12f; result.playerSpeed *= 1.15f; result.scoreMultiplier *= 1.08f; }
    if (config.HasMutator(MutatorId::ScarceRecovery)) { result.healthDropChance *= 0.30f; result.scoreMultiplier *= 1.14f; }
    if (config.HasMutator(MutatorId::Eventful)) { result.eventInterval *= 0.58f; result.scoreMultiplier *= 1.12f; }
    if (config.HasMutator(MutatorId::ChaoticWaves)) { result.specialWaveInterval *= 0.58f; result.scoreMultiplier *= 1.12f; }

    if (config.challenge == ChallengeId::TheSwarm) result.forceSwarm = true;
    if (config.challenge == ChallengeId::Titans) result.forceDurable = true;
    if (config.challenge == ChallengeId::BloodMoon) result.preferBloodMoon = true;
    if (config.challenge == ChallengeId::BossHunter) { result.bossHunter = true; result.bossHp *= 1.25f; result.minibossHp *= 1.25f; }

    const int safeCycle = std::clamp(endlessCycle, 1, 8);
    if (config.endless && safeCycle > 1) {
        const float cycle = static_cast<float>(safeCycle - 1);
        result.enemyHp *= 1.0f + std::min(1.40f, cycle * 0.22f);
        result.enemyDamage *= 1.0f + std::min(0.90f, cycle * 0.14f);
        result.spawnInterval *= std::max(0.72f, 1.0f - cycle * 0.05f);
        result.eliteChance *= 1.0f + std::min(1.5f, cycle * 0.22f);
        result.bossHp *= 1.0f + std::min(1.8f, cycle * 0.30f);
    }
    result.enemySpeed = std::clamp(result.enemySpeed, 0.8f, 1.25f);
    result.spawnInterval = std::clamp(result.spawnInterval, 0.32f, 1.6f);
    result.spawnPressure = std::clamp(result.spawnPressure, 0.5f, 1.8f);
    result.healthDropChance = std::clamp(result.healthDropChance, 0.10f, 2.0f);
    result.scoreMultiplier = std::clamp(result.scoreMultiplier, 1.0f, 8.0f);
    return result;
}

ScoreBreakdown CalculateSurvivalScore(int eligibleKills, int elites, int minibosses,
                                      int bosses, float elapsedTime,
                                      const SurvivalRunConfig& unresolved) {
    const SurvivalRunConfig config = ResolveSurvivalConfig(unresolved);
    ScoreBreakdown result{};
    result.baseScore = static_cast<long long>(std::max(0, eligibleKills)) * 10LL +
                       static_cast<long long>(std::max(0, elites)) * 75LL +
                       static_cast<long long>(std::max(0, minibosses)) * 300LL +
                       static_cast<long long>(std::max(0, bosses)) * 1200LL +
                       static_cast<long long>(std::max(0.0f, elapsedTime)) * 2LL;
    result.difficultyMultiplier = config.difficulty == DifficultyTier::Normal ? 1.0f :
                                  (config.difficulty == DifficultyTier::Hard ? 1.25f : 1.60f);
    result.ascensionMultiplier = 1.0f + 0.05f * config.ascension;
    float combined = BuildRunModifiers(config).scoreMultiplier;
    result.mutatorMultiplier = combined / (result.difficultyMultiplier * result.ascensionMultiplier);
    result.finalScore = static_cast<long long>(std::llround(result.baseScore * combined));
    return result;
}
