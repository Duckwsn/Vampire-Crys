#pragma once

#include <array>

#include "raylib.h"

struct Chest {
    Vector2 position{};
    float radius = 22.0f;
    bool active = false;
};

class ChestManager {
public:
    void Reset();
    bool Spawn(Vector2 position);
    bool Update(Vector2 playerPosition, float playerRadius);
    void Draw() const;
    int ActiveCount() const { return activeCount_; }
private:
    std::array<Chest, 4> chests_{};
    int activeCount_ = 0;
};
