#pragma once

#include <array>
#include <string>
#include <vector>

#include "entities/Player.h"
#include "game_modes/SurvivalEndgame.h"
#include "systems/AreaEffectManager.h"
#include "systems/AudioManager.h"
#include "systems/BossManager.h"
#include "systems/ChestManager.h"
#include "systems/EnemyManager.h"
#include "systems/FloatingTextManager.h"
#include "systems/LootSystem.h"
#include "systems/ParticleManager.h"
#include "systems/PickupManager.h"
#include "systems/ProjectileManager.h"
#include "systems/ScreenShakeManager.h"
#include "systems/WeaponManager.h"
#include "systems/XPOrbManager.h"

class WorldNavigation;

enum class UpgradeKind { Weapon, Passive };

struct UpgradeChoice {
    UpgradeKind kind = UpgradeKind::Weapon;
    WeaponType weapon = WeaponType::ArcBolt;
    PassiveType passive = PassiveType::SwiftBoots;
    Rarity rarity = Rarity::Common;
    std::string name;
    std::string typeLabel;
    std::string description;
    std::string effect;
    int currentLevel = 0;
    int newLevel = 1;
};

struct RunStatistics {
    float timeSurvived = 0.0f;
    int levelReached = 1;
    int enemiesKilled = 0;
    int elitesKilled = 0;
    int xpCollected = 0;
    double damageDealt = 0.0;
    double damageTaken = 0.0;
    int pickupsCollected = 0;
    int bossesKilled = 0;
    int minibossesKilled = 0;
    int scoreEligibleKills = 0;
    int evolutionsObtained = 0;
    int chestsOpened = 0;
    double damageToBosses = 0.0;
};

enum class ChestRewardKind { Evolution, WeaponUpgrade, PassiveUpgrade, Recovery, StageRecovery, Experience };

struct ChestRewardChoice {
    ChestRewardKind kind = ChestRewardKind::Recovery;
    WeaponType weapon = WeaponType::ArcBolt;
    PassiveType passive = PassiveType::SwiftBoots;
    std::string name;
    std::string description;
    Rarity rarity = Rarity::Common;
};

class GameSession {
public:
    void ResetSharedRunSystems();
    void Reset();
    void SetSelectedCharacter(CharacterId character) { selectedCharacter_ = character; }
    CharacterId SelectedCharacter() const { return selectedCharacter_; }
    void SetSurvivalRunConfig(SurvivalRunConfig config) {
        survivalConfig_ = ResolveSurvivalConfig(config);
        runModifiers_ = BuildRunModifiers(survivalConfig_);
    }
    const SurvivalRunConfig& SurvivalConfig() const { return survivalConfig_; }
    const RunModifiers& RuntimeModifiers() const { return runModifiers_; }
    ScoreBreakdown CurrentScore() const;
    void SetEndlessCyclesCompleted(int cycles) { endlessCyclesCompleted_ = std::max(0, cycles); }
    int EndlessCyclesCompleted() const { return endlessCyclesCompleted_; }
    void ConfigureBossScaling(float hp, float damage) { bosses_.SetSpawnMultipliers(hp, damage); }
    void Update(float deltaTime);
    void DrawWorld() const;
    void DrawCollisionDebug() const;
    void SetWorldNavigation(const WorldNavigation* navigation) {
        navigation_ = navigation;
        bosses_.SetNavigation(navigation);
    }
    const WorldNavigation* Navigation() const { return navigation_; }
    Vector2 ConstrainCamera(Vector2 target, Vector2 viewportHalfSize) const;
    void SetPlayerPosition(Vector2 position);
    void ResetStageCombatSystems();
    void PrepareExpeditionStageReward(bool eliteQuality);

    bool IsGameOver() const { return player_.IsDead(); }
    bool HasPendingLevelUp() const { return pendingLevelUps_ > 0; }
    void PrepareUpgradeChoices();
    void SelectUpgrade(int index);
    bool HasPendingChestReward() const { return pendingChestReward_; }
    void SelectChestReward(int index);
    bool HasPendingWeaponReplacement() const { return pendingWeaponReplacement_; }
    WeaponType PendingReplacementWeapon() const { return pendingReplacementWeapon_; }
    bool ConfirmWeaponReplacement(int slot);
    void CancelWeaponReplacement() { pendingWeaponReplacement_ = false; replacementFromLevelUp_ = false; }
    bool ReplacementFromLevelUp() const { return replacementFromLevelUp_; }
    void SetManualCombat(bool enabled) {
        manualCombat_ = enabled;
        weapons_.SetWeaponSlotLimit(enabled ? 2 : WeaponManager::MaxWeaponSlots);
    }
    bool IsManualCombat() const { return manualCombat_; }
    const AttackRequest& CurrentAttackRequest() const { return attackRequest_; }
    void SetAttackRequest(const AttackRequest& request) { attackRequest_ = request; }
    void GrantDebugXP(int amount);
    void DebugKillAll();
    void DebugSpawnBoss(BossType type);
    void DebugDamageBoss(float healthFraction);
    void DebugPrepareEvolution(WeaponType type);
    bool DebugGrantWeapon(WeaponType type) { return weapons_.AddOrUpgradeWeapon(type); }
    void DebugSpawnChest();
    void ClearBossHazards();
    void AdvanceRunTime(float seconds, float maximum);
    bool HasBossDeathThisFrame() const { return bossDeathThisFrameValid_; }
    const BossDeathEvent& BossDeathThisFrame() const { return bossDeathThisFrame_; }
    bool HasMinibossDeathThisFrame() const { return minibossDeathThisFrame_; }
    EnemyType MinibossDeathTypeThisFrame() const { return minibossDeathTypeThisFrame_; }
    void SetEnemySpeedMultiplier(float value) {
        enemySpeedMultiplier_ = std::clamp(value * runModifiers_.enemySpeed, 0.5f, 2.0f);
    }
    void GrantEvolutionOpportunity(Vector2 position);
    void SetAudioManager(AudioManager* audio) {
        audio_ = audio;
        projectiles_.SetAudioManager(audio);
        weapons_.SetAudioManager(audio);
    }

    const Player& GetPlayer() const { return player_; }
    Player& GetPlayer() { return player_; }
    const EnemyManager& Enemies() const { return enemies_; }
    EnemyManager& Enemies() { return enemies_; }
    const ProjectileManager& Projectiles() const { return projectiles_; }
    ProjectileManager& Projectiles() { return projectiles_; }
    const XPOrbManager& XPOrbs() const { return xpOrbs_; }
    XPOrbManager& XPOrbs() { return xpOrbs_; }
    const WeaponManager& Loadout() const { return weapons_; }
    WeaponManager& Loadout() { return weapons_; }
    const std::array<UpgradeChoice, 3>& UpgradeChoices() const { return choices_; }
    int UpgradeChoiceCount() const { return choiceCount_; }
    float ElapsedTime() const { return elapsedTime_; }
    int Kills() const { return kills_; }
    const PickupManager& Pickups() const { return pickups_; }
    PickupManager& Pickups() { return pickups_; }
    const ParticleManager& Particles() const { return particles_; }
    const RunStatistics& Statistics() const { return statistics_; }
    const FloatingTextManager& FloatingTexts() const { return floatingTexts_; }
    const ScreenShakeManager& ScreenShake() const { return screenShake_; }
    const BossManager& Bosses() const { return bosses_; }
    BossManager& Bosses() { return bosses_; }
    const ChestManager& Chests() const { return chests_; }
    ChestManager& Chests() { return chests_; }
    const std::array<ChestRewardChoice, 3>& ChestChoices() const { return chestChoices_; }
    int ChestChoiceCount() const { return chestChoiceCount_; }
    float UpdateMilliseconds() const { return updateMilliseconds_; }
    float DrawMilliseconds() const { return drawMilliseconds_; }
    void SetUpdateMilliseconds(float value) { updateMilliseconds_ = value; }
    void SetDrawMilliseconds(float value) { drawMilliseconds_ = value; }

private:
    void ResolveProjectileCollisions();
    void ResolveEnemyPlayerCollisions();
    void ResolveExplosionEvents();
    void ResolveEnemyBlastEvents();
    void ConsumeEnemyDeaths();
    void ResolvePickups(float deltaTime);
    void ConsumeBossDeath();
    void PrepareChestRewards();
    void ConsumeDamageFeedback();
    void AddXP(int amount);

    Player player_;
    EnemyManager enemies_;
    ProjectileManager projectiles_;
    AreaEffectManager areas_;
    XPOrbManager xpOrbs_;
    WeaponManager weapons_;
    BossManager bosses_;
    ChestManager chests_;
    LootSystem loot_;
    PickupManager pickups_;
    ParticleManager particles_;
    FloatingTextManager floatingTexts_;
    ScreenShakeManager screenShake_;
    RunStatistics statistics_{};
    std::array<UpgradeChoice, 3> choices_{};
    std::array<ChestRewardChoice, 3> chestChoices_{};
    float elapsedTime_ = 0.0f;
    int kills_ = 0;
    int pendingLevelUps_ = 0;
    int choiceCount_ = 0;
    int chestChoiceCount_ = 0;
    bool pendingChestReward_ = false;
    bool pendingWeaponReplacement_ = false;
    bool replacementFromLevelUp_ = false;
    WeaponType pendingReplacementWeapon_ = WeaponType::ArcBolt;
    bool manualCombat_ = false;
    AttackRequest attackRequest_{};
    BossDeathEvent bossDeathThisFrame_{};
    bool bossDeathThisFrameValid_ = false;
    bool minibossDeathThisFrame_ = false;
    EnemyType minibossDeathTypeThisFrame_ = EnemyType::GraveWarden;
    std::vector<int> queryScratch_;
    float updateMilliseconds_ = 0.0f;
    float drawMilliseconds_ = 0.0f;
    float playerTrailTimer_ = 0.0f;
    AudioManager* audio_ = nullptr;
    float enemySpeedMultiplier_ = 1.0f;
    CharacterId selectedCharacter_ = CharacterId::Hunter;
    SurvivalRunConfig survivalConfig_{};
    RunModifiers runModifiers_{};
    int endlessCyclesCompleted_ = 0;
    const WorldNavigation* navigation_ = nullptr;
    double carriedEnemyDamage_ = 0.0;
    double carriedBossDamage_ = 0.0;
};
