#pragma once

#include <string>

#include "raylib.h"

enum class WeaponType {
    ArcBolt, FlameRing, GuardianOrbs, SpectralFan, VoidLance, ThunderCannon,
    SoulScythe, FrostShards, BloodNeedles, GravityWell, Count
};
enum class PassiveType { SwiftBoots, PowerCore, ChronoGear, ExpansionRune, MagnetStone, VitalHeart, IronShell, LuckyCharm,
                         FocusCrystal, EmberCore, OrbitalEngine, WindSigil, PiercingEye, TitanCore,
                         ExecutionerSigil, FrozenHeart, SoulHarvest, BloodPact, GraviticCore, SoulChain, Count };
enum class EvolvedWeaponType {
    None, StormArc, InfernoHalo, CelestialGuard, PhantomBarrage, AbyssSpear, TempestCannon,
    ReapersCovenant, AbsoluteZero, CrimsonSwarm, Singularity
};
enum class Rarity { Common, Uncommon, Rare, Epic };
enum class ProjectileOwner { Player, Enemy, Boss };
enum class DamageSource { PlayerProjectile, PlayerArea, PlayerOrbital, EnemyContact, EnemyProjectile, EnemyExplosion, BossAttack };
enum class EnemyType {
    Ghoul, Swarmer, Brute, Cultist, Bomber,
    ShieldedAcolyte, Wraith, Necromancer, BoneMinion,
    GraveWarden, VoidStalker, Count
};
enum class EnemyVariant { None, Frenzied, Armored, Empowered };
enum class SpawnSource { Director, SpecialWave, Necromancer, EliteHunt, Miniboss };

struct DamageFeedbackEvent {
    Vector2 position{};
    float amount = 0.0f;
    bool critical = false;
};

struct WeaponDefinition {
    WeaponType type{};
    const char* name = "";
    const char* description = "";
    Rarity rarity = Rarity::Common;
    Color color = WHITE;
};

struct WeaponStats {
    float damage = 0.0f;
    float cooldown = 1.0f;
    int projectileCount = 1;
    float projectileSpeed = 0.0f;
    float area = 1.0f;
    float duration = 0.0f;
    int piercing = 0;
    float knockback = 0.0f;
    int ticks = 1;
    float orbitSpeed = 0.0f;
};

struct PassiveDefinition {
    PassiveType type{};
    const char* name = "";
    const char* description = "";
    Rarity rarity = Rarity::Common;
    Color color = WHITE;
};

struct WeaponEvolutionDefinition {
    WeaponType baseWeapon{};
    PassiveType requiredPassive{};
    EvolvedWeaponType evolvedWeapon = EvolvedWeaponType::None;
    const char* name = "";
    const char* behavior = "";
};

struct EnemyDefinition {
    const char* name = "";
    float maxHP = 1.0f;
    float moveSpeed = 1.0f;
    float contactDamage = 1.0f;
    int xpReward = 1;
    float radius = 10.0f;
    float knockbackResistance = 0.0f;
    float attackRange = 0.0f;
    float preferredRange = 0.0f;
    float attackCooldown = 0.0f;
    float projectileDamage = 0.0f;
    float projectileSpeed = 0.0f;
    Color color = WHITE;
};

const WeaponDefinition& GetWeaponDefinition(WeaponType type);
WeaponStats GetWeaponStats(WeaponType type, int level);
std::string GetWeaponLevelEffect(WeaponType type, int newLevel);

const PassiveDefinition& GetPassiveDefinition(PassiveType type);
std::string GetPassiveLevelEffect(PassiveType type, int newLevel);
const WeaponEvolutionDefinition& GetEvolutionDefinition(WeaponType baseWeapon);
const char* EvolvedWeaponName(EvolvedWeaponType type);

const EnemyDefinition& GetEnemyDefinition(EnemyType type);

const char* RarityName(Rarity rarity);
Color RarityColor(Rarity rarity);
float RarityWeight(Rarity rarity, float luck);
