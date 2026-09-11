#pragma once

#include <vector>

#include "raylib.h"

struct XPOrb {
    Vector2 position{};
    int value = 0;
    float radius = 0.0f;
    bool attracted = false;
    bool fastAttract = false;
    bool active = false;
};

class XPOrbManager {
public:
    XPOrbManager();

    void Reset();
    void Spawn(Vector2 position, int value);
    int Update(float deltaTime, Vector2 playerPosition, float playerRadius,
               float magnetRadius);
    void Draw() const;
    void AttractAll();
    int ActiveCount() const { return activeCount_; }

private:
    std::vector<XPOrb> orbs_;
    int activeCount_ = 0;
    bool poolWarningEmitted_ = false;
};
