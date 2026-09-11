#include "systems/LootSystem.h"
#include <algorithm>
#include "raylib.h"

bool LootSystem::Roll(float luck, bool elite, PickupType& result, float healthChanceMultiplier) const {
    const float luckMultiplier = ChanceMultiplier(luck);
    const float health = (elite ? 0.04f : 0.005f) * luckMultiplier *
                         std::clamp(healthChanceMultiplier, 0.1f, 2.0f);
    const float magnet = (elite ? 0.02f : 0.0025f) * luckMultiplier;
    const float bomb = (elite ? 0.01f : 0.001f) * luckMultiplier;
    const float roll = static_cast<float>(GetRandomValue(0, 999999)) / 1000000.0f;
    if (roll < health) { result = PickupType::Health; return true; }
    if (roll < health + magnet) { result = PickupType::Magnet; return true; }
    if (roll < health + magnet + bomb) { result = PickupType::Bomb; return true; }
    return false;
}

float LootSystem::ChanceMultiplier(float luck) const {
    return std::min(2.0f, 1.0f + std::max(0.0f, luck) * 0.35f);
}
