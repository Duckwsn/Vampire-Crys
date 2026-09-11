#pragma once

#include <algorithm>
#include <array>
#include <vector>

#include "gameplay/Definitions.h"
#include "gameplay/Characters.h"
#include "raylib.h"

class AreaEffectManager;
class AudioManager;
class BossManager;
class EnemyManager;
class Player;
struct PlayerStats;
class ProjectileManager;

struct WeaponInstance {
    WeaponType type = WeaponType::ArcBolt;
    int level = 0;
    float cooldownTimer = 0.0f;
    float phase = 0.0f;
    EvolvedWeaponType evolved = EvolvedWeaponType::None;
};

struct PassiveInstance {
    PassiveType type = PassiveType::SwiftBoots;
    int level = 0;
};

struct AttackRequest {
    Vector2 aimDirection{1.0f, 0.0f};
    Vector2 aimWorld{};
    bool primaryHeld = false;
    bool secondaryHeld = false;
};

class WeaponManager {
public:
    static constexpr int MaxWeaponSlots = 6;
    static constexpr int MaxPassiveSlots = 6;
    static constexpr int MaxWeaponLevel = 8;
    static constexpr int MaxPassiveLevel = 5;

    void Reset(WeaponType startingWeapon = WeaponType::ArcBolt);
    void SetCharacter(CharacterId character) { character_ = character; }
    void Update(float deltaTime, Player& player, EnemyManager& enemies,
                ProjectileManager& projectiles, AreaEffectManager& areas,
                BossManager* bosses = nullptr);
    void UpdateManual(float deltaTime, const AttackRequest& request, Player& player,
                      EnemyManager& enemies, ProjectileManager& projectiles,
                      AreaEffectManager& areas, BossManager* bosses = nullptr);
    void DrawOrbitals(Vector2 playerPosition, const PlayerStats& stats) const;
    void SetAudioManager(AudioManager* audio) { audio_ = audio; }

    bool HasWeapon(WeaponType type) const;
    int WeaponLevel(WeaponType type) const;
    bool AddOrUpgradeWeapon(WeaponType type);
    bool ReplaceWeapon(int slot, WeaponType type);
    bool SwapWeaponSlots();
    void SetWeaponSlotLimit(int limit) { weaponSlotLimit_ = std::clamp(limit, 1, MaxWeaponSlots); }
    int WeaponSlotLimit() const { return weaponSlotLimit_; }
    float SlotCooldown(int slot) const;
    bool HasPassive(PassiveType type) const;
    int PassiveLevel(PassiveType type) const;
    bool AddOrUpgradePassive(PassiveType type, PlayerStats& stats);
    bool IsEvolutionEligible(WeaponType type) const;
    bool EvolveWeapon(WeaponType type);
    bool IsEvolved(WeaponType type) const;
    EvolvedWeaponType Evolution(WeaponType type) const;
    int EvolutionCount() const;
    int EligibleEvolutionCount() const;

    int WeaponCount() const { return weaponCount_; }
    int PassiveCount() const { return passiveCount_; }
    const std::array<WeaponInstance, MaxWeaponSlots>& Weapons() const { return weapons_; }
    const std::array<PassiveInstance, MaxPassiveSlots>& Passives() const { return passives_; }

private:
    void FireLinearWeapon(WeaponInstance& weapon, Vector2 origin, const PlayerStats& stats,
                          const EnemyManager& enemies, ProjectileManager& projectiles,
                          float spreadDegrees, bool explosive, BossManager* bosses);
    void UpdateFlameRing(WeaponInstance& weapon, Vector2 origin, const PlayerStats& stats,
                         AreaEffectManager& areas);
    void UpdateGuardianOrbs(WeaponInstance& weapon, float deltaTime, Vector2 origin,
                            const PlayerStats& stats, EnemyManager& enemies, BossManager* bosses,
                            bool damageEnabled = true);
    void TickTimers(float deltaTime);
    void FireManualWeapon(int slot, const AttackRequest& request, Player& player,
                          EnemyManager& enemies, ProjectileManager& projectiles,
                          AreaEffectManager& areas, BossManager* bosses);
    void UpdateEvolvedWeapon(WeaponInstance& weapon, float deltaTime, Player& player,
                             EnemyManager& enemies, ProjectileManager& projectiles,
                             AreaEffectManager& areas, BossManager* bosses);
    void FireSoulScythe(WeaponInstance& weapon, Player& player, EnemyManager& enemies,
                        BossManager* bosses, bool evolved);
    void FireFrostShards(WeaponInstance& weapon, Vector2 origin, const PlayerStats& stats,
                         const EnemyManager& enemies, ProjectileManager& projectiles,
                         AreaEffectManager& areas, BossManager* bosses, bool evolved);
    void FireBloodNeedles(WeaponInstance& weapon, Vector2 origin, const PlayerStats& stats,
                          EnemyManager& enemies, ProjectileManager& projectiles,
                          BossManager* bosses, bool evolved);
    void SpawnGravityWell(WeaponInstance& weapon, Vector2 origin, const PlayerStats& stats,
                          const EnemyManager& enemies, AreaEffectManager& areas,
                          BossManager* bosses, bool evolved);
    float RollDamage(float baseDamage, const PlayerStats& stats, bool* critical = nullptr) const;
    void ApplyPassiveLevel(PassiveType type, PlayerStats& stats);

    std::array<WeaponInstance, MaxWeaponSlots> weapons_{};
    std::array<PassiveInstance, MaxPassiveSlots> passives_{};
    int weaponCount_ = 0;
    int passiveCount_ = 0;
    int weaponSlotLimit_ = MaxWeaponSlots;
    std::vector<int> queryScratch_;
    struct ChainSegment { Vector2 start{}; Vector2 end{}; float lifetime = 0.0f; };
    std::array<ChainSegment, 16> chainSegments_{};
    struct SweepVisual { Vector2 center{}; float radius = 0.0f; float startAngle = 0.0f;
                         float endAngle = 0.0f; float lifetime = 0.0f; Color color = WHITE; };
    std::array<SweepVisual, 8> sweepVisuals_{};
    AudioManager* audio_ = nullptr;
    CharacterId character_ = CharacterId::Hunter;
};
