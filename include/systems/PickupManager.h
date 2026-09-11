#pragma once

#include <vector>
#include "raylib.h"

enum class PickupType { Health, Magnet, Bomb };

struct Pickup {
    Vector2 position{};
    PickupType type = PickupType::Health;
    float radius = 12.0f;
    float lifetime = 30.0f;
    bool active = false;
};

struct PickupResult { int health = 0; int magnet = 0; int bomb = 0; };

class PickupManager {
public:
    PickupManager();
    void Reset();
    bool Spawn(Vector2 position, PickupType type);
    PickupResult Update(float deltaTime, Vector2 playerPosition, float playerRadius);
    void Draw() const;
    int ActiveCount() const { return activeCount_; }
private:
    std::vector<Pickup> pickups_;
    int activeCount_ = 0;
    bool poolWarningEmitted_ = false;
};
