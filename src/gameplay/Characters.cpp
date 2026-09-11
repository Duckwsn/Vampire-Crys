#include "gameplay/Characters.h"

namespace {
constexpr std::array<CharacterDefinition, static_cast<std::size_t>(CharacterId::Count)> Characters{{
    {CharacterId::Hunter, "HUNTER", WeaponType::ArcBolt, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
     0.0f, 0.0f, 0.0f, CharacterTrait::MarkedPrey, "MARKED PREY",
     "+15% damage against enemies above 80% HP.", Color{92, 210, 255, 255}},
    {CharacterId::Pyromancer, "PYROMANCER", WeaponType::FlameRing, 0.90f, 1.0f, 1.0f, 1.10f, 1.10f,
     0.0f, 0.0f, 0.0f, CharacterTrait::BurningMomentum, "BURNING MOMENTUM",
     "Five AoE hits grant +12% area for 3 seconds.", Color{255, 100, 55, 255}},
    {CharacterId::Sentinel, "SENTINEL", WeaponType::GuardianOrbs, 1.20f, 0.92f, 1.0f, 1.0f, 1.0f,
     2.0f, 0.0f, 0.0f, CharacterTrait::Fortified, "FORTIFIED",
     "After 5 seconds unharmed, the next hit deals 35% less damage.", Color{245, 205, 80, 255}},
    {CharacterId::Occultist, "OCCULTIST", WeaponType::VoidLance, 0.88f, 1.02f, 1.0f, 1.0f, 1.0f,
     0.0f, 0.35f, 0.04f, CharacterTrait::ForbiddenKnowledge, "FORBIDDEN KNOWLEDGE",
     "+0.35 Luck and +4% critical chance at the cost of 12% HP.", Color{196, 90, 255, 255}},
}};
}

const CharacterDefinition& GetCharacterDefinition(CharacterId id) {
    return Characters[static_cast<std::size_t>(id)];
}

const char* CharacterName(CharacterId id) { return GetCharacterDefinition(id).name; }
