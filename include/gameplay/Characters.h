#pragma once

#include <array>
#include <cstddef>

#include "gameplay/Definitions.h"

enum class CharacterId { Hunter, Pyromancer, Sentinel, Occultist, Count };
enum class CharacterTrait { MarkedPrey, BurningMomentum, Fortified, ForbiddenKnowledge };

struct CharacterDefinition {
    CharacterId id = CharacterId::Hunter;
    const char* name = "";
    WeaponType startingWeapon = WeaponType::ArcBolt;
    float maxHPMultiplier = 1.0f;
    float moveSpeedMultiplier = 1.0f;
    float damageMultiplier = 1.0f;
    float areaMultiplier = 1.0f;
    float durationMultiplier = 1.0f;
    float armorBonus = 0.0f;
    float luckBonus = 0.0f;
    float criticalBonus = 0.0f;
    CharacterTrait trait = CharacterTrait::MarkedPrey;
    const char* traitName = "";
    const char* traitDescription = "";
    Color color = WHITE;
};

const CharacterDefinition& GetCharacterDefinition(CharacterId id);
const char* CharacterName(CharacterId id);
