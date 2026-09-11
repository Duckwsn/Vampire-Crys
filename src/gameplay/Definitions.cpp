#include "gameplay/Definitions.h"

#include <algorithm>
#include <array>

namespace {
constexpr std::array<WeaponDefinition, static_cast<std::size_t>(WeaponType::Count)> Weapons{{
    {WeaponType::ArcBolt, "ARC BOLT", "Bolts seek the nearest enemy.", Rarity::Common, Color{130, 205, 255, 255}},
    {WeaponType::FlameRing, "FLAME RING", "A timed ring burns nearby crowds.", Rarity::Uncommon, Color{255, 118, 62, 255}},
    {WeaponType::GuardianOrbs, "GUARDIAN ORBS", "Orbitals damage enemies on contact.", Rarity::Rare, Color{190, 135, 255, 255}},
    {WeaponType::SpectralFan, "SPECTRAL FAN", "Spreads shots toward the nearest enemy.", Rarity::Common, Color{108, 245, 205, 255}},
    {WeaponType::VoidLance, "VOID LANCE", "A fast lance pierces enemy lines.", Rarity::Rare, Color{204, 93, 255, 255}},
    {WeaponType::ThunderCannon, "THUNDER CANNON", "A heavy shot explodes on impact.", Rarity::Epic, Color{255, 224, 82, 255}},
    {WeaponType::SoulScythe, "SOUL SCYTHE", "Sweeps a high-damage spectral arc nearby.", Rarity::Common, Color{117, 238, 190, 255}},
    {WeaponType::FrostShards, "FROST SHARDS", "Fires a controlled shard burst that slows.", Rarity::Uncommon, Color{115, 220, 255, 255}},
    {WeaponType::BloodNeedles, "BLOOD NEEDLES", "Rapid needles track and divide among targets.", Rarity::Common, Color{255, 75, 115, 255}},
    {WeaponType::GravityWell, "GRAVITY WELL", "Creates a damaging field that pulls normal enemies.", Rarity::Rare, Color{157, 91, 255, 255}},
}};

constexpr std::array<PassiveDefinition, static_cast<std::size_t>(PassiveType::Count)> Passives{{
    {PassiveType::SwiftBoots, "SWIFT BOOTS", "Increases movement speed.", Rarity::Common, SKYBLUE},
    {PassiveType::PowerCore, "POWER CORE", "Increases all player damage.", Rarity::Uncommon, RED},
    {PassiveType::ChronoGear, "CHRONO GEAR", "Reduces all weapon cooldowns.", Rarity::Rare, GOLD},
    {PassiveType::ExpansionRune, "EXPANSION RUNE", "Increases areas and orbital size.", Rarity::Uncommon, ORANGE},
    {PassiveType::MagnetStone, "MAGNET STONE", "Increases XP attraction radius.", Rarity::Common, GREEN},
    {PassiveType::VitalHeart, "VITAL HEART", "Adds max HP and regeneration.", Rarity::Uncommon, PINK},
    {PassiveType::IronShell, "IRON SHELL", "Reduces incoming damage with armor.", Rarity::Rare, LIGHTGRAY},
    {PassiveType::LuckyCharm, "LUCKY CHARM", "Improves rare choices and critical chance.", Rarity::Epic, LIME},
    {PassiveType::FocusCrystal, "FOCUS CRYSTAL", "Accelerates projectiles and sharpens criticals.", Rarity::Rare, SKYBLUE},
    {PassiveType::EmberCore, "EMBER CORE", "Expands areas and extends their duration.", Rarity::Rare, ORANGE},
    {PassiveType::OrbitalEngine, "ORBITAL ENGINE", "Improves orbital area and effect duration.", Rarity::Rare, VIOLET},
    {PassiveType::WindSigil, "WIND SIGIL", "Improves movement and projectile speed.", Rarity::Uncommon, Color{130, 245, 220, 255}},
    {PassiveType::PiercingEye, "PIERCING EYE", "Adds critical chance and global piercing.", Rarity::Epic, Color{220, 95, 255, 255}},
    {PassiveType::TitanCore, "TITAN CORE", "Adds global damage and maximum health.", Rarity::Epic, GOLD},
    {PassiveType::ExecutionerSigil, "EXECUTIONER SIGIL", "Deals more damage to low-health enemies.", Rarity::Uncommon, Color{185, 235, 160, 255}},
    {PassiveType::FrozenHeart, "FROZEN HEART", "Improves slows and grants a little armor.", Rarity::Uncommon, Color{115, 220, 255, 255}},
    {PassiveType::SoulHarvest, "SOUL HARVEST", "Enemies grant a small amount of additional XP.", Rarity::Rare, Color{120, 245, 175, 255}},
    {PassiveType::BloodPact, "BLOOD PACT", "+8% damage but -4 maximum HP each level.", Rarity::Rare, Color{255, 70, 105, 255}},
    {PassiveType::GraviticCore, "GRAVITIC CORE", "Improves duration, area and control strength.", Rarity::Rare, Color{175, 105, 255, 255}},
    {PassiveType::SoulChain, "SOUL CHAIN", "Adds projectiles at levels 2 and 4.", Rarity::Epic, Color{230, 195, 255, 255}},
}};

constexpr std::array<WeaponEvolutionDefinition, static_cast<std::size_t>(WeaponType::Count)> Evolutions{{
    {WeaponType::ArcBolt, PassiveType::FocusCrystal, EvolvedWeaponType::StormArc, "STORM ARC", "Lightning chains through up to five distinct targets."},
    {WeaponType::FlameRing, PassiveType::EmberCore, EvolvedWeaponType::InfernoHalo, "INFERNO HALO", "A persistent halo burns nearby enemies."},
    {WeaponType::GuardianOrbs, PassiveType::OrbitalEngine, EvolvedWeaponType::CelestialGuard, "CELESTIAL GUARD", "Six empowered orbitals release radial bursts."},
    {WeaponType::SpectralFan, PassiveType::WindSigil, EvolvedWeaponType::PhantomBarrage, "PHANTOM BARRAGE", "A controlled 360-degree spectral barrage."},
    {WeaponType::VoidLance, PassiveType::PiercingEye, EvolvedWeaponType::AbyssSpear, "ABYSS SPEAR", "A wide, long-lived spear with extreme piercing."},
    {WeaponType::ThunderCannon, PassiveType::TitanCore, EvolvedWeaponType::TempestCannon, "TEMPEST CANNON", "Impacts call three secondary lightning explosions."},
    {WeaponType::SoulScythe, PassiveType::ExecutionerSigil, EvolvedWeaponType::ReapersCovenant, "REAPER'S COVENANT", "Dual sweeps and periodic full-circle reaping."},
    {WeaponType::FrostShards, PassiveType::FrozenHeart, EvolvedWeaponType::AbsoluteZero, "ABSOLUTE ZERO", "Large shard bursts alternate with a frost nova."},
    {WeaponType::BloodNeedles, PassiveType::BloodPact, EvolvedWeaponType::CrimsonSwarm, "CRIMSON SWARM", "Large rapid volleys distribute across nearby targets."},
    {WeaponType::GravityWell, PassiveType::GraviticCore, EvolvedWeaponType::Singularity, "SINGULARITY", "A larger well pulls harder and pulses at its center."},
}};

constexpr std::array<EnemyDefinition, static_cast<std::size_t>(EnemyType::Count)> Enemies{{
    {"Ghoul", 32.0f, 66.0f, 9.0f, 4, 17.0f, 0.10f, 0, 0, 0, 0, 0, Color{132, 205, 100, 255}},
    {"Swarmer", 14.0f, 118.0f, 6.0f, 3, 11.0f, 0.0f, 0, 0, 0, 0, 0, Color{235, 185, 78, 255}},
    {"Brute", 105.0f, 39.0f, 18.0f, 12, 29.0f, 0.65f, 0, 0, 0, 0, 0, Color{174, 79, 78, 255}},
    {"Cultist", 42.0f, 54.0f, 8.0f, 9, 16.0f, 0.20f, 390.0f, 275.0f, 2.2f, 10.0f, 245.0f, Color{165, 105, 215, 255}},
    {"Bomber", 48.0f, 62.0f, 8.0f, 10, 19.0f, 0.15f, 0, 0, 0, 0, 0, Color{244, 103, 76, 255}},
    {"Shielded Acolyte", 72.0f, 52.0f, 11.0f, 13, 21.0f, 0.42f, 0, 0, 0, 0, 0, Color{80, 174, 224, 255}},
    {"Wraith", 45.0f, 82.0f, 10.0f, 12, 16.0f, 0.05f, 0, 0, 0, 0, 0, Color{146, 121, 224, 255}},
    {"Necromancer", 68.0f, 45.0f, 8.0f, 18, 19.0f, 0.30f, 520.0f, 345.0f, 4.8f, 0, 0, Color{103, 215, 153, 255}},
    {"Bone Minion", 13.0f, 104.0f, 5.0f, 1, 10.0f, 0.0f, 0, 0, 0, 0, 0, Color{214, 216, 184, 255}},
    {"Grave Warden", 1500.0f, 48.0f, 14.0f, 120, 42.0f, 0.90f, 0, 0, 3.2f, 0, 0, Color{184, 140, 74, 255}},
    {"Void Stalker", 1350.0f, 76.0f, 14.0f, 120, 34.0f, 0.72f, 650.0f, 270.0f, 2.8f, 11.0f, 315.0f, Color{204, 72, 242, 255}},
}};
} // namespace

const WeaponDefinition& GetWeaponDefinition(WeaponType type) {
    return Weapons[static_cast<std::size_t>(type)];
}

WeaponStats GetWeaponStats(WeaponType type, int level) {
    level = std::clamp(level, 1, 8);
    WeaponStats stats{};
    switch (type) {
        case WeaponType::ArcBolt:
            stats = {13.0f, 0.72f, 1, 560.0f, 6.0f, 1.8f, 0, 25.0f, 1, 0.0f};
            if (level >= 2) stats.damage *= 1.25f;
            if (level >= 3) stats.cooldown *= 0.88f;
            if (level >= 4) ++stats.projectileCount;
            if (level >= 5) stats.projectileSpeed *= 1.25f;
            if (level >= 6) ++stats.piercing;
            if (level >= 7) stats.damage *= 1.30f;
            if (level >= 8) { ++stats.projectileCount; ++stats.piercing; }
            break;
        case WeaponType::FlameRing:
            stats = {10.0f, 3.4f, 0, 0.0f, 105.0f, 0.55f, 0, 18.0f, 2, 0.0f};
            if (level >= 2) stats.damage *= 1.20f;
            if (level >= 3) stats.area *= 1.15f;
            if (level >= 4) stats.cooldown *= 0.90f;
            if (level >= 5) ++stats.ticks;
            if (level >= 6) stats.area *= 1.20f;
            if (level >= 7) stats.damage *= 1.30f;
            if (level >= 8) { stats.cooldown *= 0.85f; stats.duration += 0.25f; ++stats.ticks; }
            break;
        case WeaponType::GuardianOrbs:
            stats = {8.0f, 0.0f, 1, 0.0f, 13.0f, 0.0f, 0, 14.0f, 1, 1.65f};
            if (level >= 2) stats.damage *= 1.30f;
            if (level >= 3) ++stats.projectileCount;
            if (level >= 4) stats.orbitSpeed *= 1.30f;
            if (level >= 5) stats.area *= 1.30f;
            if (level >= 6) ++stats.projectileCount;
            if (level >= 7) stats.damage *= 1.35f;
            if (level >= 8) { ++stats.projectileCount; stats.duration = 18.0f; }
            break;
        case WeaponType::SpectralFan:
            stats = {8.0f, 1.65f, 3, 430.0f, 5.0f, 1.45f, 0, 12.0f, 1, 42.0f};
            if (level >= 2) stats.damage *= 1.25f;
            if (level >= 3) ++stats.projectileCount;
            if (level >= 4) stats.orbitSpeed = 52.0f;
            if (level >= 5) ++stats.projectileCount;
            if (level >= 6) ++stats.piercing;
            if (level >= 7) stats.cooldown *= 0.85f;
            if (level >= 8) stats.projectileCount += 2;
            break;
        case WeaponType::VoidLance:
            stats = {28.0f, 2.25f, 1, 760.0f, 9.0f, 1.8f, 4, 55.0f, 1, 0.0f};
            if (level >= 2) stats.damage *= 1.30f;
            if (level >= 3) ++stats.piercing;
            if (level >= 4) stats.area *= 1.35f;
            if (level >= 5) stats.projectileSpeed *= 1.25f;
            if (level >= 6) stats.piercing += 2;
            if (level >= 7) stats.cooldown *= 0.82f;
            if (level >= 8) { stats.damage *= 1.35f; stats.area *= 1.25f; }
            break;
        case WeaponType::ThunderCannon:
            stats = {42.0f, 4.2f, 1, 260.0f, 82.0f, 2.8f, 0, 75.0f, 1, 0.0f};
            if (level >= 2) stats.damage *= 1.25f;
            if (level >= 3) stats.area *= 1.20f;
            if (level >= 4) stats.cooldown *= 0.88f;
            if (level >= 5) stats.projectileSpeed *= 1.25f;
            if (level >= 6) stats.damage *= 1.30f;
            if (level >= 7) stats.area *= 1.25f;
            if (level >= 8) ++stats.projectileCount;
            break;
        case WeaponType::SoulScythe:
            stats = {38.0f, 1.55f, 1, 0.0f, 132.0f, 0.22f, 0, 34.0f, 1, 105.0f};
            if (level >= 2) stats.damage *= 1.22f;
            if (level >= 3) stats.area *= 1.15f;
            if (level >= 4) stats.orbitSpeed = 130.0f;
            if (level >= 5) stats.cooldown *= 0.84f;
            if (level >= 6) stats.damage *= 1.28f;
            if (level >= 7) stats.area *= 1.18f;
            if (level >= 8) stats.projectileCount = 2;
            break;
        case WeaponType::FrostShards:
            stats = {9.0f, 1.28f, 3, 500.0f, 4.5f, 2.0f, 0, 8.0f, 1, 24.0f};
            if (level >= 2) stats.damage *= 1.22f;
            if (level >= 3) ++stats.projectileCount;
            if (level >= 4) stats.cooldown *= 0.88f;
            if (level >= 5) stats.projectileSpeed *= 1.18f;
            if (level >= 6) ++stats.projectileCount;
            if (level >= 7) stats.damage *= 1.30f;
            if (level >= 8) { ++stats.projectileCount; ++stats.piercing; }
            break;
        case WeaponType::BloodNeedles:
            stats = {5.5f, 0.46f, 2, 540.0f, 3.2f, 2.25f, 0, 4.0f, 1, 10.0f};
            if (level >= 2) stats.damage *= 1.20f;
            if (level >= 3) ++stats.projectileCount;
            if (level >= 4) stats.cooldown *= 0.84f;
            if (level >= 5) stats.projectileSpeed *= 1.18f;
            if (level >= 6) ++stats.projectileCount;
            if (level >= 7) stats.damage *= 1.28f;
            if (level >= 8) { ++stats.projectileCount; stats.cooldown *= 0.82f; }
            break;
        case WeaponType::GravityWell:
            stats = {11.0f, 4.8f, 1, 0.0f, 145.0f, 3.0f, 0, 0.0f, 8, 0.0f};
            if (level >= 2) stats.damage *= 1.20f;
            if (level >= 3) stats.area *= 1.15f;
            if (level >= 4) stats.duration *= 1.20f;
            if (level >= 5) stats.cooldown *= 0.85f;
            if (level >= 6) stats.damage *= 1.30f;
            if (level >= 7) stats.area *= 1.20f;
            if (level >= 8) { stats.ticks += 3; stats.duration *= 1.15f; }
            break;
        case WeaponType::Count:
            break;
    }
    return stats;
}

std::string GetWeaponLevelEffect(WeaponType type, int newLevel) {
    if (newLevel <= 1) return GetWeaponDefinition(type).description;
    static const char* Arc[] = {"", "", "+25% damage", "-12% cooldown", "+1 projectile", "+25% projectile speed", "+1 piercing", "+30% damage", "+1 projectile and +1 piercing"};
    static const char* Flame[] = {"", "", "+20% damage", "+15% area", "-10% cooldown", "+1 damage tick", "+20% area", "+30% damage", "-15% cooldown, longer duration and +1 tick"};
    static const char* Orbs[] = {"", "", "+30% damage", "+1 orb", "+30% orbit speed", "+30% orb size", "+1 orb", "+35% damage", "+1 orb and wider orbit"};
    static const char* Fan[] = {"", "", "+25% damage", "+1 projectile", "wider 52 degree spread", "+1 projectile", "+1 piercing", "-15% cooldown", "+2 projectiles"};
    static const char* Lance[] = {"", "", "+30% damage", "+1 piercing", "+35% width", "+25% speed", "+2 piercing", "-18% cooldown", "+35% damage and +25% width"};
    static const char* Cannon[] = {"", "", "+25% damage", "+20% explosion radius", "-12% cooldown", "+25% projectile speed", "+30% damage", "+25% explosion radius", "+1 projectile"};
    static const char* Scythe[] = {"", "", "+22% damage", "+15% radius", "+25 degrees arc", "-16% cooldown", "+28% damage", "+18% radius", "+1 opposite sweep"};
    static const char* Frost[] = {"", "", "+22% damage", "+1 shard", "-12% cooldown", "+18% shard speed", "+1 shard", "+30% damage", "+1 shard and +1 piercing"};
    static const char* Needles[] = {"", "", "+20% damage", "+1 needle", "-16% cooldown", "+18% needle speed", "+1 needle", "+28% damage", "+1 needle and -18% cooldown"};
    static const char* Gravity[] = {"", "", "+20% damage", "+15% radius", "+20% duration", "-15% cooldown", "+30% damage", "+20% radius", "+3 ticks and +15% duration"};
    const char** table = Arc;
    switch (type) {
        case WeaponType::FlameRing: table = Flame; break;
        case WeaponType::GuardianOrbs: table = Orbs; break;
        case WeaponType::SpectralFan: table = Fan; break;
        case WeaponType::VoidLance: table = Lance; break;
        case WeaponType::ThunderCannon: table = Cannon; break;
        case WeaponType::SoulScythe: table = Scythe; break;
        case WeaponType::FrostShards: table = Frost; break;
        case WeaponType::BloodNeedles: table = Needles; break;
        case WeaponType::GravityWell: table = Gravity; break;
        default: break;
    }
    return table[std::clamp(newLevel, 1, 8)];
}

const PassiveDefinition& GetPassiveDefinition(PassiveType type) {
    return Passives[static_cast<std::size_t>(type)];
}

std::string GetPassiveLevelEffect(PassiveType type, int) {
    switch (type) {
        case PassiveType::SwiftBoots: return "+5% base movement speed";
        case PassiveType::PowerCore: return "+10% global damage";
        case PassiveType::ChronoGear: return "7% faster global cooldowns";
        case PassiveType::ExpansionRune: return "+10% area";
        case PassiveType::MagnetStone: return "+30 XP magnet radius";
        case PassiveType::VitalHeart: return "+15 max HP, heal 15, +0.2 HP/s";
        case PassiveType::IronShell: return "+1 armor";
        case PassiveType::LuckyCharm: return "+0.2 luck and +1% critical chance";
        case PassiveType::FocusCrystal: return "+10% projectile speed and +1% critical chance";
        case PassiveType::EmberCore: return "+6% area and +6% duration";
        case PassiveType::OrbitalEngine: return "+5% area and +8% duration";
        case PassiveType::WindSigil: return "+3% movement and +6% projectile speed";
        case PassiveType::PiercingEye: return "+1.5% critical chance and +1 piercing";
        case PassiveType::TitanCore: return "+5% global damage and +8 max HP";
        case PassiveType::ExecutionerSigil: return "+6% damage against enemies below 30% HP";
        case PassiveType::FrozenHeart: return "+8% slow effectiveness and +0.2 armor";
        case PassiveType::SoulHarvest: return "+4% XP from enemy deaths";
        case PassiveType::BloodPact: return "+8% damage, -4 max HP";
        case PassiveType::GraviticCore: return "+6% duration, +3% area and +10% control";
        case PassiveType::SoulChain: return "Projectile bonus at passive levels 2 and 4";
        case PassiveType::Count: return "";
    }
    return "";
}

const WeaponEvolutionDefinition& GetEvolutionDefinition(WeaponType baseWeapon) {
    return Evolutions[static_cast<std::size_t>(baseWeapon)];
}

const char* EvolvedWeaponName(EvolvedWeaponType type) {
    if (type == EvolvedWeaponType::None) return "";
    for (const auto& definition : Evolutions)
        if (definition.evolvedWeapon == type) return definition.name;
    return "EVOLVED";
}

const EnemyDefinition& GetEnemyDefinition(EnemyType type) {
    return Enemies[static_cast<std::size_t>(type)];
}

const char* RarityName(Rarity rarity) {
    switch (rarity) {
        case Rarity::Common: return "Common";
        case Rarity::Uncommon: return "Uncommon";
        case Rarity::Rare: return "Rare";
        case Rarity::Epic: return "Epic";
    }
    return "Common";
}

Color RarityColor(Rarity rarity) {
    switch (rarity) {
        case Rarity::Common: return Color{210, 214, 220, 255};
        case Rarity::Uncommon: return Color{91, 222, 125, 255};
        case Rarity::Rare: return Color{90, 160, 255, 255};
        case Rarity::Epic: return Color{205, 105, 255, 255};
    }
    return WHITE;
}

float RarityWeight(Rarity rarity, float luck) {
    luck = std::clamp(luck, 0.0f, 3.0f);
    switch (rarity) {
        case Rarity::Common: return std::max(20.0f, 60.0f - luck * 12.0f);
        case Rarity::Uncommon: return 25.0f + luck * 2.0f;
        case Rarity::Rare: return 12.0f + luck * 6.0f;
        case Rarity::Epic: return 3.0f + luck * 4.0f;
    }
    return 1.0f;
}
