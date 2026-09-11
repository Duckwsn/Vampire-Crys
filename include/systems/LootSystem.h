#pragma once
#include "systems/PickupManager.h"

class LootSystem {
public:
    bool Roll(float luck, bool elite, PickupType& result,
              float healthChanceMultiplier = 1.0f) const;
    float ChanceMultiplier(float luck) const;
};
